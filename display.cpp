#include "display.h"
#include "config.h"
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

namespace Display {
  static hd44780_I2Cexp lcd(Config::LCD_ADDRESS);
  static bool ready = false;
  static bool lit = false;
  static uint32_t updateAt = 0;
  static char lines[4][21] = {};
  static int row = 4;

  void off() {
    if (ready) {
      lcd.noDisplay();
      lcd.noBacklight();
    }
    lit = false;
    row = 4;
  }

  void begin() {
    ready = lcd.begin(20, 4) == 0;
    off();
  }

  bool available() {
    return ready;
  }

  void poll(bool active, const char *mode, float temperatureF,
            float agreement, const char *presence, uint32_t remainingMs,
            bool servoFault) {
    if (!ready) {
      return;
    }
    if (!active) {
      if (lit) {
        off();
      }
      return;
    }

    if (!lit) {
      lcd.clear();
      lit = true;
      updateAt = millis() - Config::LCD_MS;
    }

    if (row == 4 && uint32_t(millis() - updateAt) >= Config::LCD_MS) {
      updateAt = millis();
      snprintf(lines[0], sizeof(lines[0]), "Mode: %s", mode);
      if (servoFault) {
        snprintf(lines[0], sizeof(lines[0]), "SERVO FAULT");
      }

      char temperatureText[10] = "NA";
      char agreementText[10] = "NA";
      if (isfinite(temperatureF)) {
        snprintf(temperatureText, sizeof(temperatureText), "%.1f", double(temperatureF));
      }
      if (isfinite(agreement)) {
        snprintf(agreementText, sizeof(agreementText), "%.1f%%", double(agreement));
      }
      snprintf(lines[1], sizeof(lines[1]), "T:%sF A:%s", temperatureText, agreementText);
      snprintf(lines[2], sizeof(lines[2]), "Presence: %s", presence);
      uint32_t seconds = remainingMs / 1000;
      if (remainingMs % 1000 != 0) {
        seconds += 1;
      }
      snprintf(lines[3], sizeof(lines[3]), "Remaining: %02lu:%02lu",
               (unsigned long)(seconds / 60), (unsigned long)(seconds % 60));
      row = 0;
    }

    if (row < 4) {
      char padded[21];
      memset(padded, ' ', 20);
      padded[20] = '\0';
      memcpy(padded, lines[row], strlen(lines[row]));
      lcd.setCursor(0, row);
      lcd.print(padded);
      row += 1;

      if (row == 4) {
        lcd.display();
        lcd.backlight();
      }
    }
  }
}
