#pragma once
namespace Thermistor {
float readRawF(int pin); // Calibrated, unfiltered; feed this to the shared filter.
extern int lastAdc;
float readF(int pin);
float kalmanFilter(float measurement);
void printValue(float value, int digits);
}
