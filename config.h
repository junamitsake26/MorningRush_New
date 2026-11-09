#pragma once
#include <stdint.h>

namespace Config {
  // Touch Cap Settings
  constexpr int TOUCH_OUT = 36;
  constexpr int TOUCH_IN = 35;
  constexpr bool TOUCH_ENABLED = true;
  constexpr float TOUCH_PRESS_US = 200.0f;
  constexpr uint32_t TOUCH_HOLD_MS = 2000;

  // ESP32 setup
  constexpr bool N16R8_MODULE = true;
  constexpr bool ALLOW_RESERVED_TOUCH_TEST = true;

  // Analog Mux & ADC
  constexpr int MUX_SELECT = 1;
  constexpr int ADC = 6;
  constexpr uint32_t ACTIVE_ANALOG_MS = 50;
  constexpr uint32_t IDLE_ANALOG_MS = 150;

  // I2C pins & addresses
  constexpr int SDA = 40;
  constexpr int SCL = 39;
  constexpr uint8_t BMP_ADDRESS = 0x77;
  constexpr uint8_t IR_ADDRESS = 0x5A;
  constexpr uint8_t LCD_ADDRESS = 0x27;

  // Radar pins & timing
  constexpr int RADAR_RX = 18;
  constexpr int RADAR_TX = 17;
  constexpr uint32_t RADAR_ACK_MS = 500;

  // Power & system timing
  constexpr int POWER_ENABLE = 45;
  constexpr uint32_t STARTUP_MS = 2000;
  constexpr uint32_t RETRY_MS = 3000;
  constexpr uint32_t ABSENCE_MS = 100000;

  // Human Presence & Agreement
  constexpr uint32_t PRESENCE_FRESH_MS = 1000;
  constexpr uint32_t ALIGN_MS = 300;
  constexpr uint32_t AGREEMENT_WINDOW_MS = 5000;
  constexpr uint16_t AGREEMENT_MIN_PAIRS = 10;
  constexpr uint32_t IR_WARMUP_MS = 3000;

  // Servo Settings
  constexpr int SERVO_ON = 11;
  constexpr int SERVO_OFF = 12;
  constexpr uint32_t SERVO_MOVE_MS = 800;
  constexpr uint32_t SERVO_ZERO_US = 500;
  constexpr uint32_t SERVO_180_US = 2500;
  constexpr uint32_t SERVO_FRAME_US = 20000;
  constexpr uint32_t SERVO_ON_REST_DEG = 90;
  constexpr uint32_t SERVO_OFF_REST_DEG = 90;
  constexpr uint32_t SERVO_ON_PRESS_DEG = 0;
  constexpr uint32_t SERVO_OFF_PRESS_DEG = 0;
  constexpr uint32_t SERVO_PHASE_MS = 300;

  // LDR Rising Light Detection
  constexpr bool LAMP_RATE_CHECK = false;
  constexpr float LAMP_RISE_V = 0.35f;
  constexpr uint32_t LAMP_WINDOW_MS = 600;
  constexpr float LIGHT_THRESHOLD_V = 1.6f;

  // Temperature readings & filter
  constexpr uint32_t BMP_MS = 500;
  constexpr uint32_t TEMP_FRESH_MS = 5000;
  constexpr float THERM_VARIANCE = 1.6f;
  constexpr float BMP_VARIANCE = 0.25f;
  constexpr float TEMP_Q_PER_SEC = 0.04f;

  // LCD & Serial
  constexpr uint32_t LCD_MS = 250;
  constexpr uint32_t LOG_MS = 1000;

  // WiFi & ThingSpeak
  constexpr uint32_t UPLOAD_MS = 20000;
  constexpr uint32_t WIFI_RETRY_MS = 15000;
}

// touch pin check
static_assert(!Config::TOUCH_ENABLED || !Config::N16R8_MODULE ||
                  Config::ALLOW_RESERVED_TOUCH_TEST ||
                  ((Config::TOUCH_OUT < 35 || Config::TOUCH_OUT > 37) &&
                   (Config::TOUCH_IN < 35 || Config::TOUCH_IN > 37)),
              "N16R8 reserves GPIO35-37. Verify module and explicitly change touch wiring first.");
