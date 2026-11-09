#include "ld2410c_interpreter.h"

namespace LD2410CInterpreter {
  bool acknowledgment = false;
  uint16_t command = 0;
  uint16_t status = 0;
  uint8_t target = 0;
  uint16_t movingDistance = 0;
  uint16_t staticDistance = 0;
  uint16_t detectionDistance = 0;
  uint8_t movingEnergy = 0;
  uint8_t staticEnergy = 0;

  static uint8_t message[96] = {};
  static int byteCount = 0;

  void reset() {
    byteCount = 0;
  }

  // radar sends low byte first, then high byte
  static uint16_t readNumber(int position) {
    return message[position] + uint16_t(message[position + 1]) * 256;
  }

  // move buffer forward one byte to look for next message header
  static void removeFirstByte() {
    for (int i = 1; i < byteCount; i += 1) {
      message[i - 1] = message[i];
    }
    byteCount -= 1;
  }

  bool readByte(uint8_t incomingByte) {
    if (byteCount == 96) {
      removeFirstByte();
    }
    message[byteCount] = incomingByte;
    byteCount += 1;

    // find either a measurement header or setup reply header
    while (byteCount >= 4) {
      bool report = false;
      bool reply = false;
      if (message[0] == 0xF4 && message[1] == 0xF3 && message[2] == 0xF2 && message[3] == 0xF1) {
        report = true;
      }
      if (message[0] == 0xFD && message[1] == 0xFC && message[2] == 0xFB && message[3] == 0xFA) {
        reply = true;
      }

      // if not received propper info, cycle through
      if (!report && !reply) {
        removeFirstByte();
        continue;
      }
      if (byteCount < 6) {
        return false; // wait for message length bytes
      }

      int payloadLength = readNumber(4);
      int messageLength = payloadLength + 10;
      int minimumLength = 13;
      if (reply) {
        minimumLength = 4;
      }

      //check total length
      if (payloadLength < minimumLength || messageLength > 96) {
        removeFirstByte();
        continue;
      }
      if (byteCount < messageLength) {
        return false; // rest of message arrives on later calls
      }

      int footer = messageLength - 4;
      bool footerMatches = false;
      
      // check byte format
      
      if (reply) {
        if (message[footer] == 4 && message[footer + 1] == 3 && message[footer + 2] == 2 && message[footer + 3] == 1) {
          footerMatches = true;
        }
      } else {
        if (message[footer] == 0xF8 && message[footer + 1] == 0xF7 && message[footer + 2] == 0xF6 && message[footer + 3] == 0xF5) {
          footerMatches = true;
        }
      }

      // check footer matches
      if (!footerMatches) {
        removeFirstByte();
        continue;
      }

      acknowledgment = reply;
      bool usableMessage = false;
      if (reply) {
        command = readNumber(6);
        status = readNumber(8);
        usableMessage = true;
      } else {
        // data type, start marker, target, readings, end marker
        if (payloadLength == 13 && message[6] == 2 && message[7] == 0xAA && message[8] <= 3 && message[17] == 0x55 && message[18] == 0) {
          target = message[8];
          movingDistance = readNumber(9);
          movingEnergy = message[11];
          staticDistance = readNumber(12);
          staticEnergy = message[14];
          detectionDistance = readNumber(15);
          usableMessage = true;
        }
      }

      // remove read bytes & keep reamaining
      int remainingBytes = byteCount - messageLength;
      for (int i = 0; i < remainingBytes; i += 1) {
        message[i] = message[messageLength + i];
      }
      byteCount = remainingBytes;
      if (usableMessage) {
        return true;
      }
    }
    return false;
  }
}
