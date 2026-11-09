#pragma once
#include <math.h>
#include <stdint.h>

namespace Sensors {
  extern float ldr_v;
  extern bool light_decision;
  extern float rawTempF;
  extern float filteredTempF;
  extern float bmpTempF;
  extern float pressureHpa;
  extern float thermostatTempF;

  extern uint16_t kineticDistanceCm;
  extern uint16_t staticDistanceCm;
  extern int16_t kineticEnergy;
  extern int16_t staticEnergy;
  extern bool newReading;

  void readRaw(bool active);
  void readBMP();
  float readThermostat(float filteredThermistorF, float bmpTempF);
  bool startPresence();
  void stopPresence();
  void readPresence();
}
