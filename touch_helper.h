#pragma once
#include <Arduino.h>

namespace Touch {
  bool enableTouch();
  void checkTouch();
  bool takePressRelease();
  bool takeHold();
  float averageDelayUs();
  void clearPressHold();
}
