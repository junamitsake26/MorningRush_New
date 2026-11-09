#pragma once
#include <Arduino.h>

namespace Power {
void begin();
void enable();
void disable(); 
bool stabilized(uint32_t now);
bool enabled();
} // namespace Power

namespace Servos {
void begin(); 
bool prepare();
bool on();
bool off();
bool poll(); 
bool busy();
void stop();
extern uint32_t onCommands, offCommands;
} // namespace Servos
