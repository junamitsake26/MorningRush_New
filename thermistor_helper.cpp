#include <Arduino.h>
#include <math.h>
#include "thermistor_helper.h"
namespace Thermistor {
int lastAdc = 0;
const float v_ADC_Max = 3.3f, bits_ADC = 4095.0f;
const float G_B = 0.751f, v_B_Offset = 0.499f;
const float v_INA_Ref = 2.55f, G_INA = 8.46f;
const float V_Sup_B = 5.00f, R_null = 0.789f;
const float R_P = 3.96f, v_Minus = 2.51f;
const float BMP_Offset = 3.9f;

// kalman filter setup. Room temp. of 77.7 degF
float estimateError = 2.0f, filteredTemp = 77.7f;

// turn the divider voltage into resistance; reject values outside its range
float vPlusToResistance(float v_Plus) {
  if (v_Plus <= 0 || v_Plus >= V_Sup_B) { //guarantees voltage is within divider
    return NAN;
  }

  // Solve V+ = 5V * R_P / (R_null + R_P).
  float R_Lin = v_Plus * R_null / (V_Sup_B - v_Plus); //guarantees resistance is within range of R_P.
  if (R_Lin <= 0 || R_Lin >= R_P){
    return NAN;
  } 

  return R_Lin; // Linearized resistance (R_Therm || 3.96 kΩ)
}

// use the fitted resistance line and calibration offset to get temperature
float resistanceToTemperature(float resistance) {
  // R_Lin = -0.0138 * T_R + 1.8553 directly for temperature.
  return (1.8553f - resistance) / 0.0138f + BMP_Offset;
}

// smooth one thermistor reading; main algorithm uses the shared filter instead
float kalmanFilter(float measurement) {

  static bool initialized = false;
  if (!initialized) { // start the filter from the first reading
    initialized = true;
    filteredTemp = measurement;
    return filteredTemp;
  }

  estimateError += 0.02f; // account for uncertainty from noise variance (needs to be fine tuned)
  float gain = estimateError / (estimateError + 1.6f); // gain found from error (from last loop) & noise variance. stronger trust in prior measurement = less gain (b/c original is good)
  filteredTemp += gain * (measurement - filteredTemp); // new temp is gained difference from current & previous
  estimateError *= 1.0f - gain; // reduce esitmate error variance by complement of gain (1 - g)

  return filteredTemp;
}

// print the value, or N/A if it is unavailable
void printValue(float value, int digits) {
  if (isnan(value)) {
    Serial.print("N/A");
  }
  else {
    Serial.print(value, digits);
  } 
}


// undo the amplifier stages, then convert resistance to temperature
float readRawF(int pin) {
  int adc = analogRead(pin);
  lastAdc = adc;
  if (adc <= 0 || adc >= 4095) return NAN; // Saturated ADC is not a usable temperature.
  // Bias amplifier output reaches the shared ADC through the mux.
  float V_o = adc * v_ADC_Max / bits_ADC;
  // V_out = 0.751 * V_INA - 0.499 -> V_INA = (V_out + 0.499) / 0.751
  float vInAmp = (V_o + v_B_Offset) / G_B;
  // V_INA = v_INA_Ref + G_INA * V_b
  float v_B = (vInAmp - v_INA_Ref) / G_INA;
  // V_b = V+ - V- -> V+ = V_b + V-
  float v_Plus = v_B + v_Minus;
  float resistance = vPlusToResistance(v_Plus);
  float raw = NAN;

  if (!isnan(resistance)) { // ensures only accepted resistance range used
    raw = resistanceToTemperature(resistance);
  }

  return raw;
}
// optional thermistor-only filtered reading
float readF(int pin) {
  float raw = readRawF(pin);
  return isnan(raw) ? NAN : kalmanFilter(raw);
}
}


