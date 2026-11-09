#pragma once
#include <math.h>

namespace Agreement {
  constexpr int capacity = 10;

  extern int kineticDistances[capacity];
  extern int staticDistances[capacity];
  extern int kineticEnergies[capacity];
  extern int staticEnergies[capacity];
  extern int countPairTotal;
  extern int nextPairSpot;

  extern float kineticPercentage;
  extern float staticPercentage;

  void gatherPairs();
  void clearPairs();
  float weightPercentage(int distance);
  float agreementPercentage(int distances[], int energies[], int baselineDistance);
  void updateAgreementPercentages();
}

