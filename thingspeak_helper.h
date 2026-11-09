#pragma once
#include <Arduino.h>

namespace ThingSpeakHelper {
  bool begin(const char *ssid, const char *password, unsigned long channel, const char *writeKey);
  void submit(float temperatureF, float agreement, int state, int presence,
              int32_t remainingSeconds, bool timerRunning, uint32_t now,
              uint16_t pairCount);
  int lastResult();
  bool connected();
}
