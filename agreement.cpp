#include "agreement.h"
#include "sensors.h"

namespace Agreement {
  // declare the array for storing kinetic and static presence data
  int kineticDistances[capacity] = {};
  int staticDistances[capacity] = {};
  int kineticEnergies[capacity] = {};
  int staticEnergies[capacity] = {};
  int countPairTotal = 0;
  int nextPairSpot = 0;

  // declare percentage (starte with nothing).
  float kineticPercentage = NAN;
  float staticPercentage = NAN;

  // for starting & shutting down
  void clearPairs() {
    countPairTotal = 0;
    nextPairSpot = 0;
    kineticPercentage = NAN;
    staticPercentage = NAN;
  }

  // for low and high sense
  void gatherPairs() {
    Sensors::readPresence();

    if (!Sensors::newReading) {
      return;
    }

    // adds values to the arrays
    kineticDistances[nextPairSpot] = Sensors::kineticDistanceCm;
    staticDistances[nextPairSpot] = Sensors::staticDistanceCm;
    kineticEnergies[nextPairSpot] = Sensors::kineticEnergy;
    staticEnergies[nextPairSpot] = Sensors::staticEnergy;
    nextPairSpot += 1;

    if (nextPairSpot == capacity) {
      nextPairSpot = 0;
    }

    if (countPairTotal < capacity) {
      countPairTotal += 1;
    }
  }

  // if mmWave detected at greater than 200, only look at mmWave (past IR range of effectivenss)
  // if mmWave detected at less than 200, split even presence detection between both
  float weightPercentage(int distance) {
    if (distance > 200) {
      return 100;
    }
    return 50;
  }

  //Final presence confidence calculation.  Tested with NEMA WD 7-2011 described testing to find effective ranges.
  float agreementPercentage(int distances[], int energies[], int baselineDistance) {
    bool emptyRoomBaseline = true;

    // checks if within 5cm of true range value
    for (int i = 0; i < countPairTotal; i += 1) {
      int distance = distances[i];
      if (distance < baselineDistance - 5 || distance > baselineDistance + 5) {
        emptyRoomBaseline = false;
      }
    }

    // checks if around the baseline value
    bool radarPresence = false;
    if (!emptyRoomBaseline) {
      radarPresence = true;
    }

    // calculate weighted percentage of each sensor
    float confidence = 0;
    for (int i = 0; i < countPairTotal; i += 1) {
      float radarWeight = weightPercentage(distances[i]);
      float irWeight = 100 - radarWeight;

      if (radarPresence && distances[i] > 0) {
        confidence += radarWeight;
      }
      if (energies[i] > 50) {
        confidence += irWeight;
      }
    }

    // final calculations
    float finalAgreementPercentage = confidence / countPairTotal;
    return finalAgreementPercentage;
  }

  void updateAgreementPercentages() {
    if (countPairTotal < capacity) {
      return;
    }

    // NEMA WD 7-2011 Verified Distance ReadOut. Checked Against Heat Map.
    kineticPercentage = agreementPercentage(kineticDistances, kineticEnergies, 310);
    staticPercentage = agreementPercentage(staticDistances, staticEnergies, 225);
  }
}
