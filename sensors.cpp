#include "sensors.h"
#include "config.h"
#include "ldr_helper.h"
#include "thermistor_helper.h"
#include "bmp280_helper.h"
#include "ld2410c_helper.h"
#include "Adafruit_STHS34PF80.h"
#include <Arduino.h>
#include <Wire.h>

namespace Sensors {
float ldr_v = NAN;
bool light_decision = false;
float rawTempF = NAN;
float filteredTempF = NAN;
float bmpTempF = NAN;
float pressureHpa = NAN;
float thermostatTempF = NAN;
uint16_t kineticDistanceCm = 0;
uint16_t staticDistanceCm = 0;
int16_t kineticEnergy = 0;
int16_t staticEnergy = 0;
bool newReading = false;
static Adafruit_STHS34PF80 infrared;

bool startPresence() {
  LD2410C::begin();

  if (!infrared.begin(Config::IR_ADDRESS, &Wire)) {
    return false;
  }
  return infrared.setOutputDataRate(STHS34PF80_ODR_30_HZ); //command from Arduino Library
}

void stopPresence() {
  LD2410C::end();
  kineticDistanceCm = 0;
  staticDistanceCm = 0;
  kineticEnergy = 0;
  staticEnergy = 0;
  newReading = false;
}

void readRaw(bool active) {

  if (!active) { // Mux LOW: LDR.
    ldr_v = LDR::readVoltage(Config::ADC);
    light_decision = LDR::isLight(ldr_v);
  } else { // Mux HIGH: thermistor.
    rawTempF = Thermistor::readRawF(Config::ADC);
    filteredTempF = Thermistor::kalmanFilter(rawTempF);
  }

}

void readPresence() {

  newReading = false;
  LD2410C::poll();

  kineticDistanceCm = LD2410C::latestMoveDistCm;
  staticDistanceCm = LD2410C::latestStaticDistCm;

  Adafruit_STHS34PF80::CheckedReading ir;
  if (infrared.readChecked(ir) == 1) { // ensures that new reading actually received
    kineticEnergy = ir.motionRaw;
    staticEnergy = ir.presenceRaw;
    newReading = true;

    // Agreement::gatherPairs() stores these values after calling readPresence()

  }


}

void readBMP() {

  float tempC = NAN;
  pressureHpa = NAN;
  BMP280::read(tempC, pressureHpa);
  bmpTempF = tempC * 1.8f + 32.0f;

}

float readThermostat(float filteredThermistorF, float bmpTempF) {
  thermostatTempF = (filteredThermistorF + bmpTempF) / 2.0f; // averages the filtered Thermistor & BMP280
  return thermostatTempF;
}
}

