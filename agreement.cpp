#include "agreement.h"
#include "sensors.h"

namespace Agreement {
  int kineticDistances[capacity] = {};
  int staticDistances[capacity] = {};
  int kineticEnergies[capacity] = {};
  int staticEnergies[capacity] = {};
  int countPairTotal = 0;
  int nextPairSpot = 0;

  float kineticPercentage = NAN;
  float staticPercentage = NAN;

  void clearPairs() {
    countPairTotal = 0;
    nextPairSpot = 0;
    kineticPercentage = NAN;
    staticPercentage = NAN;
  }

  void gatherPairs() {
    Sensors::readPresence();

    if (!Sensors::newReading) {
      return;
    }

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

  float weightPercentage(int distance) {
    if (distance > 200) {
      return 100;
    }
    return 50;
  }

  float agreementPercentage(int distances[], int energies[], int baselineDistance) {
    bool emptyRoomBaseline = true;

    for (int i = 0; i < countPairTotal; i += 1) {
      int distance = distances[i];
      if (distance < baselineDistance - 5 || distance > baselineDistance + 5) {
        emptyRoomBaseline = false;
      }
    }

    bool radarPresence = false;
    if (!emptyRoomBaseline) {
      radarPresence = true;
    }

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
