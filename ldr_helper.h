#pragma once
namespace LDR {
float readVoltage(int pin);
bool isLight(float voltage);
bool readLight(int pin);
}
