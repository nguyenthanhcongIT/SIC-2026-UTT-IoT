#include <Arduino.h>
#include <RadioLib.h>

// ===================== CẤU HÌNH LORA (GIỐNG HỆT CODE TEST ĐANG CHẠY) =====================
#define RF_FREQUENCY          868.0
#define TX_OUTPUT_POWER       14
#define LORA_BANDWIDTH        125.0
#define LORA_SPREADING_FACTOR 7
#define LORA_CODINGRATE       5
#define LORA_PREAMBLE_LENGTH  8
#define LORA_SYNC_WORD        0x12

#define NODE_ID               0x01

// ===================== CHÂN CẢM BIẾN =====================
#define TRIG1_PIN             PA10
#define ECHO1_PIN             PB14
#define TRIG2_PIN             PB4
#define ECHO2_PIN             PB3
#define RAIN_D0_PIN           PC0

// ===================== RADIO =====================
STM32WLx radio = new STM32WLx_Module();

static const uint32_t rfswitch_pins[] = {PA4, PA5, RADIOLIB_NC, RADIOLIB_NC, RADIOLIB_NC};
static const Module::RfSwitchMode_t rfswitch_table[] = {
  {STM32WLx::MODE_IDLE,  {LOW,  LOW}},
  {STM32WLx::MODE_RX,    {HIGH, LOW}},
  {STM32WLx::MODE_TX_HP, {LOW,  HIGH}},
  END_OF_MODE_TABLE,
};

// ===================== BIẾN =====================
uint16_t dist1_cm = 0;
uint16_t dist2_cm = 0;
uint8_t  is_raining = 0;
uint16_t rain_analog_val = 0;

uint8_t lora_payload[8];

// ===================== HÀM ĐO SIÊU ÂM =====================
uint16_t Read_HCSR04_Single_cm(uint8_t trig_pin, uint8_t echo_pin) {
  digitalWrite(trig_pin, LOW);
  delayMicroseconds(2);
  digitalWrite(trig_pin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig_pin, LOW);

  unsigned long duration = pulseIn(echo_pin, HIGH, 30000);
  if (duration == 0) return 0;

  uint16_t dist = (uint16_t)(duration * 0.0343 / 2.0);
  if (dist > 400 || dist < 2) return 0;
  return dist;
}

uint16_t Read_HCSR04_cm(uint8_t trig_pin, uint8_t echo_pin) {
  uint16_t samples[5];
  for (int i = 0; i < 5; i++) {
    samples[i] = Read_HCSR04_Single_cm(trig_pin, echo_pin);
    delay(10);
  }

  // Median filter
  for (int i = 0; i < 4; i++) {
    for (int j = i + 1; j < 5; j++) {
      if (samples[i] > samples[j]) {
        uint16_t temp = samples[i];
        samples[i] = samples[j];
        samples[j] = temp;
      }
    }
  }
  return samples[2];
}

uint8_t Read_Rain_Digital() {
  return (digitalRead(RAIN_D0_PIN) == LOW) ? 1 : 0;
}

void Prepare_LoRa_Payload() {
  lora_payload[0] = NODE_ID;
  lora_payload[1] = is_raining;
  lora_payload[2] = (uint8_t)(dist1_cm >> 8);
  lora_payload[3] = (uint8_t)(dist1_cm & 0xFF);
  lora_payload[4] = (uint8_t)(dist2_cm >> 8);
  lora_payload[5] = (uint8_t)(dist2_cm & 0xFF);
  lora_payload[6] = (uint8_t)(rain_analog_val >> 8);
  lora_payload[7] = (uint8_t)(rain_analog_val & 0xFF);
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("=== LoRa-E5 Sensor TX ===");

  // GPIO cảm biến
  pinMode(TRIG1_PIN, OUTPUT);
  pinMode(ECHO1_PIN, INPUT);
  pinMode(TRIG2_PIN, OUTPUT);
  pinMode(ECHO2_PIN, INPUT);
  pinMode(RAIN_D0_PIN, INPUT_PULLUP);

  digitalWrite(TRIG1_PIN, LOW);
  digitalWrite(TRIG2_PIN, LOW);

  // Radio (giống hệt code test đang chạy)
  radio.setRfSwitchTable(rfswitch_pins, rfswitch_table);

  int state = radio.begin(RF_FREQUENCY, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                          LORA_CODINGRATE, LORA_SYNC_WORD, TX_OUTPUT_POWER,
                          LORA_PREAMBLE_LENGTH, 1.7, false);

  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("Radio init failed: ");
    Serial.println(state);
    while (1) delay(1000);
  }

  radio.setCRC(true);
  radio.setCurrentLimit(60);
  Serial.println("Radio OK - Start sensor + TX");
}

void loop() {
  // 1. Đọc cảm biến
  is_raining = Read_Rain_Digital();
  dist1_cm   = Read_HCSR04_cm(TRIG1_PIN, ECHO1_PIN);
  dist2_cm   = Read_HCSR04_cm(TRIG2_PIN, ECHO2_PIN);

  // 2. Đóng gói
  Prepare_LoRa_Payload();

  // 3. In debug
  Serial.print("Rain=");
  Serial.print(is_raining);
  Serial.print(" | D1=");
  Serial.print(dist1_cm);
  Serial.print("cm | D2=");
  Serial.print(dist2_cm);
  Serial.print("cm → ");

  // 4. Gửi LoRa
  int state = radio.transmit(lora_payload, 8);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("TX OK");
  } else {
    Serial.print("TX FAIL (");
    Serial.print(state);
    Serial.println(")");
  }

  // 5. Adaptive Sampling
  if (is_raining == 1 || dist1_cm < 35 || dist2_cm < 35) {
    delay(1000);   // Có mưa / sự cố → 1s
  } else {
    delay(3000);   // Bình thường → 3s
  }
}