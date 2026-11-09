#include <Arduino.h>
#include <Wire.h>

#include "config.h"
#include "sensors.h"
#include "agreement.h"
#include "bmp280_helper.h"
#include "touch_helper.h"
#include "power_servo.h"
#include "display.h"
#include "thingspeak_helper.h"
#include "secrets.h"

const unsigned long thingspeakChannel = 3498116;

// clear sensors staging
bool bmpReady = false;
bool touchReady = false;
bool cloudReady = false;

// FSM design
enum State {
  DISABLED,
  STARTING,
  LOW_MODE,
  HIGH_MODE,
  SHUTTING_DOWN
};

// start in Disabled with cleared variables
State state = DISABLED;
bool active = false;
bool presenceSensorsStarted = false;
bool servoOnStarted = false;
bool servoOffStarted = false;
bool absenceTimerRunning = false;
uint32_t absenceStartedAt = 0;
float selectedPercentage = NAN;
float temperature = NAN;
const char *presenceText = "UNKNOWN";
uint32_t remainingMs = Config::ABSENCE_MS;

// set mux to LDR, reset stored light conditions & wifi log
bool muxActive = false;
bool lightKnown = false;
bool previousLight = false;
uint32_t lastWifiSubmitAt = 0;

// states as text for LCD printout
const char *modeText() {
  switch (state) {
    case DISABLED:
      return "DISABLED";
    case STARTING:
      return "STARTING";
    case LOW_MODE:
      return "LOW";
    case HIGH_MODE:
      return "HIGH";
    case SHUTTING_DOWN:
      return "SHUTTING DOWN";
  }
  return "UNKNOWN";
}

void printData() {
  // System Data
  Serial.printf("ms=%lu | state=%s | presence=%s | remaining_ms=%lu | timer=%d | power=%d\n",
                (unsigned long)millis(), modeText(), presenceText,
                (unsigned long)remainingMs, absenceTimerRunning, Power::enabled());
  // Thermostat Data (thermistor + bmp)              
  Serial.printf("therm_raw_F=%.3f | therm_filtered_F=%.3f | bmp_F=%.3f | average_F=%.3f | pressure_hPa=%.3f\n",
                Sensors::rawTempF, Sensors::filteredTempF, Sensors::bmpTempF,
                temperature, Sensors::pressureHpa);
  // LDR              
  Serial.printf("light_V=%.3f | light=%d | touch_ready=%d | touch_us=%.2f\n",
                Sensors::ldr_v, Sensors::light_decision, touchReady, Touch::averageDelayUs());
  // mmWave
  Serial.printf("move_cm=%u | static_cm=%u | ir_motion=%d | ir_static=%d | new_pair=%d\n",
                (unsigned)Sensors::kineticDistanceCm, (unsigned)Sensors::staticDistanceCm,
                Sensors::kineticEnergy, Sensors::staticEnergy, Sensors::newReading);
  // IR thermal presence
  Serial.printf("agreement_pct=%.1f | kinetic_pct=%.1f | static_pct=%.1f | pairs=%d\n",
                selectedPercentage, Agreement::kineticPercentage,
                Agreement::staticPercentage, Agreement::countPairTotal);
  // Wi-Fi & Thingspeak Info
  Serial.printf("servo_on=%lu | servo_off=%lu | servo_busy=%d | lcd=%d | cloud=%d | wifi=%d | upload_result=%d\n",
                (unsigned long)Servos::onCommands, (unsigned long)Servos::offCommands,
                Servos::busy(), Display::available(), cloudReady,
                ThingSpeakHelper::connected(), ThingSpeakHelper::lastResult());
  Serial.println("----------------------------------------");
}

void startShutdown() {
  state = SHUTTING_DOWN;
  servoOffStarted = false;
  absenceTimerRunning = false;
  Touch::clearPressHold();
}

void setup() {
  Serial.begin(115200);

  // sensor power OFF & servos at LOW
  Power::begin();
  Servos::begin();

  // I2C bus for BMP280, IR & LCD
  Wire.begin(Config::SDA, Config::SCL);
  Wire.setClock(100000);
  Wire.setTimeOut(20);

  // shared ADC input. set mux LOW for LDR
  analogReadResolution(12);
  pinMode(Config::ADC, INPUT);
  digitalWrite(Config::MUX_SELECT, LOW);
  pinMode(Config::MUX_SELECT, OUTPUT);

  bmpReady = BMP280::begin();
  Display::begin(); // turn on LCD BUT light off
  touchReady = Touch::enableTouch();

  // start WiFi & thingspeak upload. my wifi & key are in secrets.h file. (no free hotspot for you)
  cloudReady = ThingSpeakHelper::begin(
      MORNINGRUSH_WIFI_SSID, MORNINGRUSH_WIFI_PASSWORD,
      thingspeakChannel, MORNINGRUSH_WRITE_KEY);

  // Note: Presence Sensors Don't Start & fun until Starting mode.
}

// Loop runs throuhg the FSM (disabled, starting, low, high, and shutting down)
void loop() {

  // grab values before checking FSM. determine press state
  Touch::checkTouch();
  bool shortPress = Touch::takePressRelease();
  bool touchHold = Touch::takeHold();
  bool risingLight = false;

  // select analog sensor. disabled = LDR, else = thermistor
  active = state != DISABLED;
  if (muxActive != active) {
    muxActive = active;
    if (muxActive) {
      digitalWrite(Config::MUX_SELECT, HIGH);
    } else {
      digitalWrite(Config::MUX_SELECT, LOW);
    }
    lightKnown = false;
  }
  
  Sensors::readRaw(active);
  if (!active) {
    if (lightKnown && !previousLight && Sensors::light_decision) {
      risingLight = true;
    }
    previousLight = Sensors::light_decision;
    lightKnown = true;
  }

  // if thermistor on, read average of filtered temp. else, only bmp (thermistor isnt on yet)
  Sensors::readBMP();
  if (active) {
    temperature = Sensors::readThermostat(Sensors::filteredTempF, Sensors::bmpTempF);
  } else {
    temperature = Sensors::bmpTempF;
  }

  // gatherPairs calls readPresence(), which also polls radar
  if (presenceSensorsStarted) {
    Agreement::gatherPairs();
    Agreement::updateAgreementPercentages();
  }

  // continue any servo movement started by FSM
  Servos::poll();

  // declare important variables from each loop: new time, cleared %, new presence decision & remaining countdown timer.
  uint32_t now = millis();
  selectedPercentage = NAN;
  presenceText = "UNKNOWN";
  remainingMs = Config::ABSENCE_MS;

  switch (state) {
    // starting state. LCD & servos powerd off, waiting for LDR or touh cap to start
    case DISABLED:
      // once detected, servo powers, presence sensor pairs cleared, & touch reset -> switch to starting
      if (risingLight || shortPress) {
        Power::enable();
        Agreement::clearPairs();
        presenceSensorsStarted = false;
        servoOnStarted = false;
        absenceTimerRunning = false;
        Touch::clearPressHold();
        state = STARTING;
      }
      break;

    // stagin state. allows presence sensors to turn on, fill uip presence pairs, and on servo fires switch on. 
    case STARTING:
      if (Power::stabilized(now)) {
        if (!presenceSensorsStarted) {
          Sensors::startPresence();
          presenceSensorsStarted = true;
        }

        if (presenceSensorsStarted && Agreement::countPairTotal == Agreement::capacity) {
          if (!servoOnStarted) {
            Servos::on();
            servoOnStarted = true;
          }
          if (servoOnStarted && !Servos::busy()) {
            state = LOW_MODE;
            absenceTimerRunning = false;
            Touch::clearPressHold();
          }
        }
      }
      break;

    // both Low & High sense use same funciton. LOW = kinetic, HIGH = static
    case LOW_MODE:
    case HIGH_MODE:
      // touch hold for 2s goes Low -> High _> Shutdown
      if (touchHold) {
        absenceTimerRunning = false;
        if (state == LOW_MODE) {
          state = HIGH_MODE;
          Agreement::clearPairs();
          Touch::clearPressHold();
        } else {
          startShutdown();
        }
        break;
      }
      // Low = Kinetic. High = Static.
      if (state == LOW_MODE) {
        selectedPercentage = Agreement::kineticPercentage;
      } else {
        selectedPercentage = Agreement::staticPercentage;
      }


      // If > 75% pairs say present, mode = PRESENT (hold absent timer). Else, mode = ABSENT (absent tiemr ticks down)
      if (!isfinite(selectedPercentage)) {
        absenceTimerRunning = false; // wait for enough pairs
      } else if (selectedPercentage > 75) {
        presenceText = "PRESENT";
        absenceTimerRunning = false;
      } else {
        presenceText = "ABSENT";
        if (!absenceTimerRunning) {
          absenceStartedAt = now;
          absenceTimerRunning = true;
        }
        // Absent timer ticks down. if reaches 0, switch to shut down
        uint32_t elapsed = now - absenceStartedAt;
        if (elapsed >= Config::ABSENCE_MS) {
          remainingMs = 0;
          startShutdown();
        } else {
          remainingMs = Config::ABSENCE_MS - elapsed;
        }
      }
      break;

    // stagig state. inverse of Starting. fires servo off, clears data, and sets back to disabled
    case SHUTTING_DOWN:
      if (!servoOffStarted && !Servos::busy()) {
        Servos::off();
        servoOffStarted = true;
      }
      if (servoOffStarted && !Servos::busy()) {
        Sensors::stopPresence();
        presenceSensorsStarted = false;
        Agreement::clearPairs();
        Servos::stop();
        Display::off();
        Power::disable();
        Touch::clearPressHold();
        state = DISABLED;
      }
      break;
  }

  active = state != DISABLED;
  Display::poll(active, modeText(), temperature, selectedPercentage,
                presenceText, remainingMs);

  // send latest values every 20 seconds, only while WiFi is connected
  now = millis();
  if (cloudReady && ThingSpeakHelper::connected() && uint32_t(now - lastWifiSubmitAt) >= Config::UPLOAD_MS) {
    lastWifiSubmitAt = now;
    int presenceValue = -1; // start with null, so theres a known dis/connected.
    if (isfinite(selectedPercentage)) {
      presenceValue = 0;
      if (selectedPercentage > 75) {
        presenceValue = 1;
      }
    }

    int32_t remainingSeconds = -1;
    if ((state == LOW_MODE || state == HIGH_MODE) && presenceValue != -1) {
      remainingSeconds = (remainingMs + 999) / 1000;
    }
    ThingSpeakHelper::submit(temperature, selectedPercentage, int(state), presenceValue,
                            remainingSeconds, absenceTimerRunning, now,
                            Agreement::countPairTotal);
  }

  printData(); // current readings & status at end of each loop. from display.
  delay(1); // give ESP32 background work a turn
}


