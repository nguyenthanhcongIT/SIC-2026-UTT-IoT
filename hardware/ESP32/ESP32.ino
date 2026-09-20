
#define TRIG_PIN 5
#define ECHO_PIN 18
#define RXD2 16
#define TXD2 17

void setup() {
  Serial.begin(115200); // Giao tiếp với máy tính để xem log
  
  // Mở Serial2 để nói chuyện với LoRa E5 (Baudrate của con E5 hôm trước test là 9600 hoặc 115200 tùy mạch, bạn để ý chỉnh lại cho đúng)
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2); 
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  Serial.println("Khoi tao he thong...");
  
  // Lệnh AT chuyển LoRa sang chế độ phát
  Serial2.println("AT+MODE=TEST");
  delay(1000); 
}

void loop() {
  // 1. Đọc khoảng cách
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  long duration = pulseIn(ECHO_PIN, HIGH);
  int distance = duration * 0.034 / 2; 
  
  Serial.print("Đo được: ");
  Serial.print(distance);
  Serial.println(" cm");

  // 2. Ép số vào lệnh AT và gửi sang LoRa
  // Tạo lệnh dạng: AT+TEST=TXLRSTR,"KhoangCach:25cm"
  String atCommand = "AT+TEST=TXLRSTR,\"KhoangCach:" + String(distance) + "cm\"";
  Serial2.println(atCommand); 
  
  Serial.println("Đã bắt LoRa phát sóng!");
  delay(3000); // Đợi 3 giây rồi làm lại
}