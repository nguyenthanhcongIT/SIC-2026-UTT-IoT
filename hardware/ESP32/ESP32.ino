// Chân cảm biến mưa
#define PIN_D0  17
#define PIN_A0  34

// Chân cảm biến siêu âm HC-SR04
#define PIN_TRIG 27
#define PIN_ECHO 25

// --- CÁC THÔNG SỐ CÀI ĐẶT THỰC TẾ ---
#define KHOANG_CACH_DAY     200  // Khoảng cách từ siêu âm xuống mặt đường cạn (cm)
#define NGUONG_BAO_NGAP     5    // Chênh lệch 5cm thì tính là bắt đầu ngập
#define ADC_SUONG_MU        3200 // A0 lớn hơn mức này chỉ là sương/hơi ẩm
#define ADC_UOT_DAM         2000 // A0 nhỏ hơn mức này auto là MƯA DẦM
#define BIEN_DO_NHAY        50   // Độ sụt giảm đột ngột báo hạt mưa rơi
#define DO_RUT_NUOC         150  // Tốc độ bay hơi/khô dần

// --- CÁC TRẠNG THÁI CỦA HỆ THỐNG ---
#define TT_NGU_DONG         0 // Bảng khô + Đường cạn -> Ngủ tiết kiệm pin
#define TT_NGHI_NGO_MUA     1 // Có nước trên bảng, nhưng đang lọc xem là sương hay lá rụng
#define TT_DANG_MUA_THAT    2 // Đang có mưa rơi thực sự
#define TT_TANH_NHUNG_NGAP  3 // Trời tạnh, nhưng đường VẪN NGẬP
#define TT_NUOC_DONG_CAN    4 // Trời tạnh, đường cạn, nhưng bảng mưa chưa khô

uint8_t  trang_thai = TT_NGU_DONG;
uint16_t a0_truoc = 4095;
uint16_t a0_thap_nhat = 4095;
uint32_t moc_thoi_gian = 0;
uint8_t  dem_xung_mua = 0;

// Hàm đọc siêu âm (trả về cm)
uint16_t docSieuAm(uint8_t trig, uint8_t echo) {
    digitalWrite(trig, LOW);
    delayMicroseconds(2);
    digitalWrite(trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig, LOW);
    
    unsigned long duration = pulseIn(echo, HIGH, 30000);
    if (duration == 0) return 0;
    return (uint16_t)(duration * 0.034 / 2);
}

void setup() {
    Serial.begin(115200);
    pinMode(PIN_D0, INPUT);
    pinMode(PIN_A0, INPUT);
    pinMode(PIN_TRIG, OUTPUT);
    pinMode(PIN_ECHO, INPUT);
    printf("--- HE THONG CANH BAO NGAP (BAN FINAL - CHONG NHIEU & QUAN LY NGU) ---\n");
}

void loop() {
    uint8_t d0_val = digitalRead(PIN_D0);
    uint16_t a0_hien_tai = analogRead(PIN_A0);
    int16_t delta = (int16_t)a0_hien_tai - (int16_t)a0_truoc;
    uint32_t now = millis();
    
    uint16_t khoang_cach_nuoc = 0;
    uint8_t duong_dang_ngap = 0; 

    // 1. SIÊU ÂM CHỈ ĐO KHI HỆ THỐNG KHÔNG NGỦ VÀ ĐÃ QUA BƯỚC LỌC NHIỄU
    if (trang_thai == TT_DANG_MUA_THAT || trang_thai == TT_TANH_NHUNG_NGAP || trang_thai == TT_NUOC_DONG_CAN) {
        khoang_cach_nuoc = docSieuAm(PIN_TRIG, PIN_ECHO);
        if (khoang_cach_nuoc > 0 && khoang_cach_nuoc <= (KHOANG_CACH_DAY - NGUONG_BAO_NGAP)) {
            duong_dang_ngap = 1; 
        }
    }

    // 2. KỊCH BẢN BẢNG CẢM BIẾN MƯA KHÔ RÁO (D0 = 1)
    if (d0_val == 1) { 
        if (duong_dang_ngap) {
            trang_thai = TT_TANH_NHUNG_NGAP;
            printf("[NGUY HIỂM] Gió thổi bảng khô, nhưng ĐƯỜNG ĐANG NGẬP (Nước: %u cm)!\n", khoang_cach_nuoc);
        } else {
            // Chỉ đi ngủ khi đường cạn VÀ bảng khô
            if (trang_thai != TT_NGU_DONG) {
                printf("[ĐI NGỦ] Trời nắng ráo, đường đã cạn. Hệ thống tắt siêu âm...\n");
            }
            trang_thai = TT_NGU_DONG;
            a0_thap_nhat = 4095;
            dem_xung_mua = 0;
        }
    } 
    // 3. KỊCH BẢN BẢNG CẢM BIẾN MƯA BỊ ƯỚT (D0 = 0)
    else {
        switch (trang_thai) {
            case TT_NGU_DONG:
                trang_thai = TT_NGHI_NGO_MUA;
                moc_thoi_gian = now;
                dem_xung_mua = 0;
                a0_thap_nhat = a0_hien_tai;
                printf("[KÍCH HOẠT] Phát hiện ẩm (A0=%u) -> Bật hệ thống phân tích...\n", a0_hien_tai);
                break;

            case TT_NGHI_NGO_MUA:
                // Lọc sương mù
                if (a0_hien_tai > ADC_SUONG_MU) {
                    printf("[LỌC NHIỄU] A0=%u (>%d) -> Chỉ là sương mù, không bật siêu âm!\n", a0_hien_tai, ADC_SUONG_MU);
                } 
                else {
                    // Lọc rác/lá rụng: Yêu cầu giọt nước rơi liên tục 2 lần hoặc ướt sũng mới xác nhận mưa
                    if (delta < -BIEN_DO_NHAY || a0_hien_tai < ADC_UOT_DAM) dem_xung_mua++;
                    
                    if (dem_xung_mua >= 2 || a0_hien_tai < ADC_UOT_DAM) {
                        trang_thai = TT_DANG_MUA_THAT;
                        moc_thoi_gian = now;
                        printf("\n>>> [XÁC NHẬN MƯA THẬT] Bắt đầu đo siêu âm... <<<\n");
                    }
                }
                break;

            case TT_DANG_MUA_THAT:
                if (a0_hien_tai < a0_thap_nhat) a0_thap_nhat = a0_hien_tai; // Ghi nhận độ ướt lớn nhất

                // Nước vẫn rơi hoặc đang bão hòa
                if (a0_hien_tai < ADC_UOT_DAM || delta < -BIEN_DO_NHAY) {
                    moc_thoi_gian = now; 
                    printf("[ĐANG MƯA] A0=%u | Mực nước ngập: %u cm\n", a0_hien_tai, khoang_cach_nuoc);
                } 
                else {
                    // Tạnh mưa: Nước bắt đầu rút đi hoặc đứng im > 10 giây
                    if ((a0_hien_tai > a0_thap_nhat + DO_RUT_NUOC) || (now - moc_thoi_gian > 10000)) {
                        trang_thai = duong_dang_ngap ? TT_TANH_NHUNG_NGAP : TT_NUOC_DONG_CAN;
                    }
                }
                break;

            case TT_TANH_NHUNG_NGAP:
            case TT_NUOC_DONG_CAN:
                // Theo dõi xem có cơn mưa rào nào ập tới lại không
                if (delta < -(BIEN_DO_NHAY * 2)) {
                    trang_thai = TT_DANG_MUA_THAT;
                    a0_thap_nhat = a0_hien_tai;
                    moc_thoi_gian = now;
                    printf("\n>>> [MƯA LẠI] Phát hiện đợt mưa mới! <<<\n");
                } else {
                    if (trang_thai == TT_TANH_NHUNG_NGAP) {
                        printf("[CẢNH BÁO] Đã tạnh mưa, nhưng ĐƯỜNG CHƯA RÚT HẾT NƯỚC: %u cm\n", khoang_cach_nuoc);
                        // Cập nhật lại nếu nước rút sạch
                        if (!duong_dang_ngap) trang_thai = TT_NUOC_DONG_CAN;
                    } else {
                        printf("[TẠNH RÁO] Đường đã cạn (0 cm), bảng còn ướt. Chờ gió thổi khô...\n");
                        // Cập nhật lại nếu cống trào ngược làm ngập
                        if (duong_dang_ngap) trang_thai = TT_TANH_NHUNG_NGAP;
                    }
                }
                break;
        }
    }

    a0_truoc = a0_hien_tai;
    delay(1000); 
}
