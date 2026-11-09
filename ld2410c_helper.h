#pragma once
#include <Arduino.h>

namespace LD2410C {
extern uint8_t latestRadarStatus, latestMoveEnergy, latestStaticEnergy;
extern uint16_t latestMoveDistCm, latestStaticDistCm, latestDetectionDistCm;
extern uint32_t lastFrameMs, validFrames, configFailures;
void begin();
void end();
void poll();
bool configured();
bool hasFreshReading();
float correctedDistanceM(uint16_t measuredCm);
} // namespace LD2410C
