#pragma once
#include <Arduino.h>

namespace Display {
  void begin();
  void off();
  bool available();
  void poll(bool active, const char *mode, float temperatureF,
            float agreement, const char *presence, uint32_t remainingMs,
            bool servoFault = false);
}
