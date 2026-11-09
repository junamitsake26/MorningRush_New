#include <Arduino.h>
#include "ldr_helper.h"

namespace LDR {
const float ADC_VREF = 3.3f;
const int ADC_MAX = 4095;
const float LIGHT_THRESHOLD_V = 1.65f;

// ADC bits to V
float readVoltage(int pin) {
  int adcRaw = analogRead(pin);
  float adcVoltage = adcRaw * (ADC_VREF / ADC_MAX);
  return adcVoltage;
}

bool isLight(float voltage) { return voltage >= LIGHT_THRESHOLD_V; }
bool readLight(int pin) { return isLight(readVoltage(pin)); }
}
