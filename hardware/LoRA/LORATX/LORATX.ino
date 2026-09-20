// #include <SPI.h>
// #include <LoRa.h>

// // Chân mặc định của dòng TTGO/Heltec phổ biến
// #define SCK     5    
// #define MISO    19   
// #define MOSI    27   
// #define SS      18   
// #define RST     14   
// #define DIO0    26   

// int counter = 0;

// void setup() {
//   Serial.begin(115200);
//   while (!Serial);
  
//   SPI.begin(SCK, MISO, MOSI, SS);
//   LoRa.setPins(SS, RST, DIO0);

//   // Thay 433E6 bằng 915E6 nếu trên anten của bạn ghi 915MHz
//   if (!LoRa.begin(433E6)) { 
//     Serial.println("Khoi tao LoRa THAT BAI!");
//     while (1);
//   }
//   Serial.println("Khoi tao LoRa THANH CONG. Bat dau phat...");
// }

// void loop() {
//   Serial.print("Dang gui goi tin: ");
//   Serial.println(counter);

//   LoRa.beginPacket();
//   LoRa.print("Test tin hieu LoRa: ");
//   LoRa.print(counter);
//   LoRa.endPacket();

//   counter++;
//   delay(2000); // Gửi 2 giây 1 lần
// }

// #define RXD2 16
// #define TXD2 17

// void setup() {
//   Serial.begin(115200);  // Giao tiếp giữa ESP32 và Máy tính
//   // Khởi tạo Serial2 giao tiếp với LoRa. (Lưu ý: Đổi 9600 thành 115200 nếu LoRa của bạn dùng baudrate cao)
//   Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2); 
//   Serial.println("San sang gui lenh AT den LoRa...");
// }

// void loop() {
//   if (Serial.available()) {
//     Serial2.write(Serial.read()); // Đẩy lệnh AT từ máy tính sang LoRa
//   }
//   if (Serial2.available()) {
//     Serial.write(Serial2.read()); // Đẩy phản hồi từ LoRa lên máy tính
//   }
// }