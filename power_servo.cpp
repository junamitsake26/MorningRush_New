#include "power_servo.h"
#include "config.h"
#include <ESP32Servo.h>

// tracks enabled status, the indicator LED & the startup delay
namespace Power {
static bool powered = false;
static uint32_t enabledAt = 0;

void begin() {
  digitalWrite(Config::POWER_ENABLE, LOW);
  pinMode(Config::POWER_ENABLE, OUTPUT);
  powered = false;
}

void enable() {
  digitalWrite(Config::POWER_ENABLE, HIGH);
  enabledAt = millis();
  powered = true;
}

// finish delay before checking presence
bool stabilized(uint32_t now) {
  return powered && uint32_t(now - enabledAt) >= Config::STARTUP_MS;
}

void disable() {
  digitalWrite(Config::POWER_ENABLE, LOW);
  powered = false;
}

bool enabled() {
  return powered;
}
} // namespace Power


// ON and OFF servos each push a button, then return to rest.
namespace Servos {
static Servo onServo;
static Servo offServo;
// idle -> rest -> press -> rest
static uint8_t phase = 0;
static int movingPin = -1;
static uint32_t phaseAt = 0;
uint32_t onCommands = 0;
uint32_t offCommands = 0;

void begin() {
  digitalWrite(Config::SERVO_ON, LOW);
  pinMode(Config::SERVO_ON, OUTPUT);
  digitalWrite(Config::SERVO_OFF, LOW);
  pinMode(Config::SERVO_OFF, OUTPUT);
}

// Arduino Library get sets 50 Hz pulse for control
bool prepare() {
  if (!onServo.attached()) {
    onServo.setPeriodHertz(50);
    onServo.attach(Config::SERVO_ON, Config::SERVO_ZERO_US, Config::SERVO_180_US);
  }
  if (!offServo.attached()) {
    offServo.setPeriodHertz(50);
    offServo.attach(Config::SERVO_OFF, Config::SERVO_ZERO_US, Config::SERVO_180_US);
  }
  return onServo.attached() && offServo.attached();
}

static bool angle(int pin, uint32_t degrees) {
  // Adruino Library converts degrees into PWM contorl
  if (pin == Config::SERVO_ON && onServo.attached()) {
    onServo.write(degrees);
    return true;
  }
  if (pin == Config::SERVO_OFF && offServo.attached()) {
    offServo.write(degrees);
    return true;
  }
  return false;
}

static uint32_t restAngle(int pin) {
  if (pin == Config::SERVO_ON) {
    return Config::SERVO_ON_REST_DEG;
  }
  return Config::SERVO_OFF_REST_DEG;
}

// start servo at rest. loop finishes movement
static bool start(int pin) {
  if (!prepare() || busy() || !angle(pin, restAngle(pin))) {
    return false;
  }
  movingPin = pin;
  phase = 1;
  phaseAt = millis();
  return true;
}

// ON servo (bottom)
bool on() {
  if (!start(Config::SERVO_ON)) {
    return false;
  }
  ++onCommands;
  return true;
}

// OFF servo (top)
bool off() {
  if (!start(Config::SERVO_OFF)) {
    return false;
  }
  ++offCommands;
  return true;
}

bool busy() {
  return phase != 0;
}

//  rest -> press -> return
bool poll() {
  if (!phase || uint32_t(millis() - phaseAt) < Config::SERVO_PHASE_MS) {
    return true;
  }
  bool ok = true;
  if (phase == 1) {
    uint32_t pressDegrees = movingPin == Config::SERVO_ON
                                ? Config::SERVO_ON_PRESS_DEG : Config::SERVO_OFF_PRESS_DEG;
    ok = angle(movingPin, pressDegrees);
  } else if (phase == 2) {
    ok = angle(movingPin, restAngle(movingPin));
  }
  if (!ok) {
    phase = 0;
    return false;
  }
  // each movement step gets full time
  phaseAt = millis();
  phase++;
  if (phase == 4) {
    phase = 0;
  }
  return true;
}

// turn off PWM control
void stop() {
  if (onServo.attached()) {
    onServo.detach();
  }
  if (offServo.attached()) {
    offServo.detach();
  }
  phase = 0;
  pinMode(Config::SERVO_ON, INPUT);
  pinMode(Config::SERVO_OFF, INPUT);
}
} // namespace Servos
