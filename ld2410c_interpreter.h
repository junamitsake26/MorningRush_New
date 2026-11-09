#pragma once
#include <stdint.h>

namespace LD2410CInterpreter {
  extern bool acknowledgment;
  extern uint16_t command;
  extern uint16_t status;
  extern uint8_t target;
  extern uint16_t movingDistance;
  extern uint16_t staticDistance;
  extern uint16_t detectionDistance;
  extern uint8_t movingEnergy;
  extern uint8_t staticEnergy;

  void reset();
  bool readByte(uint8_t incomingByte);
}
