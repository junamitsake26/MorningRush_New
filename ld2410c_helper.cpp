#include "ld2410c_helper.h"
#include "ld2410c_interpreter.h"
#include "config.h"

namespace LD2410C {
  uint8_t latestRadarStatus = 0;
  uint8_t latestMoveEnergy = 0;
  uint8_t latestStaticEnergy = 0;
  uint16_t latestMoveDistCm = 0;
  uint16_t latestStaticDistCm = 0;
  uint16_t latestDetectionDistCm = 0;
  uint32_t lastFrameMs = 0;
  uint32_t validFrames = 0;
  uint32_t configFailures = 0;

  static bool liveSerial = false;
  static bool finsihedSetup = false;
  static bool waitingForReply = false;
  static bool haveReading = false;
  static int commandStep = 0;
  static uint16_t expectedReply = 0;

  static const uint8_t cmdEnable[] = {
    0xFD, 0xFC, 0xFB, 0xFA,
    0x04, 0x00, 0xFF, 0x00, 0x01, 0x00,
    0x04, 0x03, 0x02, 0x01
  };
  static const uint8_t cmdSetGates[] = {
    0xFD, 0xFC, 0xFB, 0xFA,
    0x14, 0x00, 0x60, 0x00,
    0x00, 0x00, 0x06, 0x00, 0x00, 0x00, // Moving gate = 6
    0x01, 0x00, 0x06, 0x00, 0x00, 0x00, // Stationary gate = 6
    0x02, 0x00, 0x05, 0x00, 0x00, 0x00, // Absence duration = 5 seconds
    0x04, 0x03, 0x02, 0x01
  };
  static const uint8_t cmdBasicMode[] = {
    0xFD, 0xFC, 0xFB, 0xFA,
    0x02, 0x00, 0x63, 0x00,
    0x04, 0x03, 0x02, 0x01
  };
  static const uint8_t cmdDisable[] = {
    0xFD, 0xFC, 0xFB, 0xFA,
    0x02, 0x00, 0xFE, 0x00,
    0x04, 0x03, 0x02, 0x01
  };

  // start serial. setup commands are sent by poll()
  void begin() {
    if (liveSerial) {
      Serial1.end();
    }
    Serial1.setRxBufferSize(1024);
    Serial1.begin(256000, SERIAL_8N1, Config::RADAR_RX, Config::RADAR_TX);
    LD2410CInterpreter::reset();
    liveSerial = true;
    finsihedSetup = false;
    waitingForReply = false;
    haveReading = false;
    commandStep = 0;
    latestRadarStatus = 0;
    latestMoveEnergy = 0;
    latestStaticEnergy = 0;
    latestMoveDistCm = 0;
    latestStaticDistCm = 0;
    latestDetectionDistCm = 0;
    lastFrameMs = 0;
  }

  void end() {
    if (liveSerial) {
      Serial1.end();
    }
    liveSerial = false;
    finsihedSetup = false;
    haveReading = false;
    waitingForReply = false;
    LD2410CInterpreter::reset();
    pinMode(Config::RADAR_TX, INPUT);
    pinMode(Config::RADAR_RX, INPUT);
  }

  // one command at a time. next command waits for matching reply
  static void sendSetupCommand() {
    if (finsihedSetup || waitingForReply) {
      return;
    }
    switch (commandStep) {
      case 0:
        Serial1.write(cmdEnable, sizeof(cmdEnable));
        expectedReply = 0x01FF;
        break;
      case 1:
        Serial1.write(cmdSetGates, sizeof(cmdSetGates));
        expectedReply = 0x0160;
        break;
      case 2:
        Serial1.write(cmdBasicMode, sizeof(cmdBasicMode));
        expectedReply = 0x0163;
        break;
      case 3:
        Serial1.write(cmdDisable, sizeof(cmdDisable));
        expectedReply = 0x01FE;
        break;
    }
    waitingForReply = true;
  }

  static void readSetupReply() {
    if (!waitingForReply || LD2410CInterpreter::command != expectedReply) {
      return;
    }
    if (LD2410CInterpreter::status != 0) {
      configFailures += 1;
      return; // leave setup stopped if radar rejects a command
    }
    waitingForReply = false;
    commandStep += 1;
    if (commandStep == 4) {
      finsihedSetup = true;
    }
  }

  static void saveReadings() {
    latestRadarStatus = LD2410CInterpreter::target;
    latestMoveDistCm = LD2410CInterpreter::movingDistance;
    latestStaticDistCm = LD2410CInterpreter::staticDistance;
    latestDetectionDistCm = LD2410CInterpreter::detectionDistance;
    latestMoveEnergy = LD2410CInterpreter::movingEnergy;
    latestStaticEnergy = LD2410CInterpreter::staticEnergy;
    lastFrameMs = millis();
    validFrames += 1;
    haveReading = true;
  }

  void poll() {
    if (!liveSerial) {
      return;
    }

    int bytesRead = 0;
    while (Serial1.available() > 0 && bytesRead < 256) {
      uint8_t incomingByte = Serial1.read();
      bytesRead += 1;
      bool completeMessage = LD2410CInterpreter::readByte(incomingByte);
      if (completeMessage) {
        if (LD2410CInterpreter::acknowledgment) {
          readSetupReply();
        } else if (finsihedSetup) {
          saveReadings();
        }
      }
    }
    sendSetupCommand();
  }

  bool configured() {
    return finsihedSetup;
  }

  bool hasFreshReading() {
    if (finsihedSetup && haveReading && uint32_t(millis() - lastFrameMs) < Config::PRESENCE_FRESH_MS) {
      return true;
    }
    return false;
  }

  float correctedDistanceM(uint16_t measuredCm) {
    float distance = (measuredCm / 100.0f - 0.378704f) / 1.010630f;
    if (distance > 0) {
      return distance;
    }
    return 0;
  }
}


