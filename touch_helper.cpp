#include "touch_helper.h"
#include "config.h"
#include <math.h>

namespace Touch {
  static constexpr uint32_t halfPeriodUs = 5000;
  static constexpr uint32_t timeoutUs = 2000;
  static constexpr int readingsPerDecision = 20;
  static bool ready = false;
  static bool highPhase = false;
  static uint32_t phaseStartedAt = 0;
  static uint32_t measuredDelayUs = timeoutUs;
  static uint32_t totalDelayUs = 0;
  static int readingCount = 0;
  static int validReadingCount = 0;
  static float latestAverageDelayUs = NAN;
  static bool held = false;
  static bool holdFired = false;
  static bool releaseEvent = false;
  static bool holdEvent = false;
  static uint32_t heldAt = 0;

  // clear current readings & set back to low
  void clearPressHold() {
    releaseEvent = false;
    holdEvent = false;
    if (held) { // if still touched, stop instnat high
      holdFired = true;
    }
    totalDelayUs = 0;
    readingCount = 0;
    validReadingCount = 0;
    digitalWrite(Config::TOUCH_OUT, LOW);
    highPhase = false;
    phaseStartedAt = micros();
  }

  // set up pins
  bool enableTouch() {
    if (!Config::TOUCH_ENABLED) {
      return false;
    }
    digitalWrite(Config::TOUCH_OUT, LOW);
    pinMode(Config::TOUCH_OUT, OUTPUT);
    pinMode(Config::TOUCH_IN, INPUT);
    held = false;
    holdFired = false;
    latestAverageDelayUs = NAN;
    clearPressHold();
    ready = true;
    return true;
  }

  // check rise value
  void checkTouch() {
    if (!ready) { // makes sure enabled
      return;
    }
    if (uint32_t(micros() - phaseStartedAt) < halfPeriodUs) { // allow full half-period; main loop keeps running
      return;
    }

    if (!highPhase) {
      measuredDelayUs = timeoutUs; // defaulsts when no rise
      bool startsLow = digitalRead(Config::TOUCH_IN) == LOW;
      uint32_t riseStartedAt = micros(); // save start time
      digitalWrite(Config::TOUCH_OUT, HIGH);
      phaseStartedAt = riseStartedAt;
      highPhase = true; // store high phase
      if (startsLow) { // input must start LOW to measure a rise
        while (uint32_t(micros() - riseStartedAt) < timeoutUs) {
          if (digitalRead(Config::TOUCH_IN) == HIGH) {
            measuredDelayUs = uint32_t(micros() - riseStartedAt);
            break;
          }
        }
      }
      return;
    }

    digitalWrite(Config::TOUCH_OUT, LOW); // set low for capacitor discharge
    phaseStartedAt = micros(); // store when LOW phase started
    highPhase = false;
    readingCount += 1; // add cycle to total 20

    if (measuredDelayUs < timeoutUs) { // only add readings captured before timeout
      totalDelayUs += measuredDelayUs;
      validReadingCount += 1;
    }
    if (readingCount < readingsPerDecision) { // keep gathering until 20 cycles completed
      return;
    }

    latestAverageDelayUs = NAN; 
    if (validReadingCount > 0) {
      latestAverageDelayUs = float(totalDelayUs) / validReadingCount; // average usable t_d
      uint32_t now = millis();
      if (latestAverageDelayUs > Config::TOUCH_PRESS_US) { // > t_d resting = touched
        if (!held) { // if stil touched, start hold timer
          held = true;
          heldAt = now; // save hold start time
        }
        if (!holdFired && uint32_t(now - heldAt) >= Config::TOUCH_HOLD_MS) { // if touched for 2 seconds, event not sent yet
          holdEvent = true; 
          holdFired = true; // make sure not re-pressed
        }
      } else { // < t_d resting = released
        if (held && !holdFired) { // if press not long enough for hold
          releaseEvent = true; // send short press
        }
        held = false;
        holdFired = false;
      }
    } else { // no usable readings; stop current hold timer
      held = false;
      holdFired = false;
    }
    // clear for next batch
    readingCount = 0; 
    validReadingCount = 0;
    totalDelayUs = 0;
  }

  // return latest average for serial output or checking threshold
  float averageDelayUs() {
    return latestAverageDelayUs;
  }

  // return short press once, then clear event so main loop doesn't repeat it
  bool takePressRelease() {
    bool result = releaseEvent;
    releaseEvent = false;
    return result;
  }

  // return hold once, then clear queued event
  bool takeHold() {
    bool result = holdEvent;
    holdEvent = false;
    return result;
  }
}
