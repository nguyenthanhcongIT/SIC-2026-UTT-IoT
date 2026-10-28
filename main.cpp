#include <Arduino.h>
#include <RadioLib.h>

#define RF_FREQUENCY          868.0
#define LORA_BANDWIDTH        125.0
#define LORA_SPREADING_FACTOR 7
#define LORA_CODINGRATE       5
#define LORA_PREAMBLE_LENGTH  8
#define LORA_SYNC_WORD        0x12

STM32WLx radio = new STM32WLx_Module();

static const uint32_t rfswitch_pins[] = {PA4, PA5, RADIOLIB_NC, RADIOLIB_NC, RADIOLIB_NC};
static const Module::RfSwitchMode_t rfswitch_table[] = {
  {STM32WLx::MODE_IDLE,  {LOW,  LOW}},
  {STM32WLx::MODE_RX,    {HIGH, LOW}},
  {STM32WLx::MODE_TX_HP, {LOW,  HIGH}},
  END_OF_MODE_TABLE,
};

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("#BOOT,LORA_E5_RX");

  radio.setRfSwitchTable(rfswitch_pins, rfswitch_table);

  int state = radio.begin(RF_FREQUENCY, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                          LORA_CODINGRATE, LORA_SYNC_WORD, 14,
                          LORA_PREAMBLE_LENGTH, 1.7, false);

  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("#ERROR,radio_init_failed,");
    Serial.println(state);
    while (1) delay(1000);
  }

  radio.setCRC(true);
  radio.setCurrentLimit(60);
  Serial.println("#RX,radio_ready");
}

void loop() {
  uint8_t payload[8];
  int state = radio.receive(payload, 8);   // Blocking (ổn định hơn)

  if (state == RADIOLIB_ERR_NONE) {
    uint8_t  node_id    = payload[0];
    uint8_t  is_raining = payload[1];
    uint16_t dist1      = ((uint16_t)payload[2] << 8) | payload[3];
    uint16_t dist2      = ((uint16_t)payload[4] << 8) | payload[5];
    uint16_t rain_adc   = ((uint16_t)payload[6] << 8) | payload[7];

    if (is_raining > 1) {
      Serial.println("#ERROR,invalid_rain_value");
      return;
    }

    Serial.print("DATA,");
    Serial.print(node_id);
    Serial.print(',');
    Serial.print(is_raining);
    Serial.print(',');
    Serial.print(dist1);
    Serial.print(',');
    Serial.print(dist2);
    Serial.print(',');
    Serial.print(rain_adc);
    Serial.print(',');
    Serial.print(radio.getRSSI(), 0);
    Serial.print(',');
    Serial.println(radio.getSNR(), 2);
  }
  else if (state != RADIOLIB_ERR_RX_TIMEOUT) {
    Serial.print("#ERROR,rx,");
    Serial.println(state);
  }
}