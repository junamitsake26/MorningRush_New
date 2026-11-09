#include "thingspeak_helper.h"
#include <WiFi.h>
#include <ThingSpeak.h>
#include <math.h>
#include <stdio.h>

namespace ThingSpeakHelper {
  static WiFiClient client;
  static unsigned long channelId = 0;
  static const char *writeApiKey = nullptr;
  static int lastHttpCode = 0;

  // start WiFi connection
  bool begin(const char *ssid, const char *password, unsigned long channel, const char *writeKey) {
    channelId = channel;
    writeApiKey = writeKey;
    client.setConnectionTimeout(3000);
    client.setTimeout(3000);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(ssid, password);
    return ThingSpeak.begin(client);
  }

  bool connected() {
    if (WiFi.status() == WL_CONNECTED) {
      return true;
    }
    return false;
  }

  // send up  to ThingSpeak Channel. .ino decides when (20s)
  void submit(float temperatureF, float agreement, int state, int presence,
              int32_t remainingSeconds, bool timerRunning, uint32_t now,
              uint16_t pairCount) {
    if (!connected()) {
      return;
    }

    // field 1 = agreement percentage (mode dependent)
    if (isfinite(agreement)) {
      ThingSpeak.setField(1, agreement);
    } else {
      ThingSpeak.setField(1, String("NA"));
    }
    // filed 2 = -1 unknown (disabled read), 0 absent, 1 present
    ThingSpeak.setField(2, presence); 
    // 1 Disabled, 2 Starting, 3 Low, 4 High, 5 Shutting Down
    ThingSpeak.setField(3, state); 

    // field 4 = thermostat temperature (avg filtered therm + bmp) in F
    if (isfinite(temperatureF)) {
      ThingSpeak.setField(4, temperatureF);
    } else {
      ThingSpeak.setField(4, String("NA"));
    }

    // field 5 = remaining absent timer seconds
    if (remainingSeconds >= 0) {
      ThingSpeak.setField(5, int(remainingSeconds));
    } else {
      ThingSpeak.setField(5, String("NA"));
    }

    char statusText[160];
    snprintf(statusText, sizeof(statusText),
             "ms=%lu timer_running=%d pairs=%u metric=weighted_detection",
             (unsigned long)now, timerRunning, (unsigned)pairCount);
    ThingSpeak.setStatus(String(statusText));

    // direct upload
    lastHttpCode = ThingSpeak.writeFields(channelId, writeApiKey);
    client.stop();
  }

  int lastResult() {
    return lastHttpCode; // 200 = ThingSpeak approved
  }
}
