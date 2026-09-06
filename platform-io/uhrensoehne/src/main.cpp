// ============================================================
//  main.cpp  –  Framework (WiFi, AP-Blink, setup, loop)
//
//  Uhr in config.h wählen:
//    #define ACTIVE_CLOCK  OBEGRANSAD
//    #define ACTIVE_CLOCK  WORDCLOCK
//    #define ACTIVE_CLOCK  TIME_2_WORDS
//    #define ACTIVE_CLOCK  CUSTOM
//
//  Hardware-Code liegt in den jeweiligen cl_*.cpp Dateien.
//  Diese Datei NICHT ändern.
// ============================================================
#include <Arduino.h>
#include "config.h"
#include "clock_hooks.h"

#if ACTIVE_CLOCK == SPRITPREIS_CYD

// Die Spritpreis-Uhr nutzt dieselben Hooks, verwaltet aber WLAN, Webserver,
// Zeitabgleich und Touch selbst in cl_spritpreis_cyd.cpp.
void setup() {
  Serial.begin(115200);
  Serial.printf("\n[BOOT] %s\n", DEVICE_NAME);
  user_setup();
}

void loop() {
  user_loop();
}

#else

#ifdef ESP32
  #include <WiFi.h>
#else
  #include <ESP8266WiFi.h>
#endif
#include <WiFiManager.h>
#include "settings.h"
#include "ntp.h"
#include "webif.h"

// ---- WiFi --------------------------------------------------
static void wifiSetup() {
  WiFi.mode(WIFI_STA);
  WiFi.hostname(DEVICE_NAME);
  WiFiManager wm;
  wm.setConnectTimeout(10);         // max 10s auf bekanntes WLAN warten
  wm.setConfigPortalBlocking(false); // non-blocking damit wir blinken können

  // Callbacks: AP gestartet / AP beendet
  wm.setAPCallback([](WiFiManager*) {
    Serial.println("[WIFI] AP-Modus aktiv – zeige AP-Anzeige");
    user_apBlink(true);
  });
  wm.setSaveConfigCallback([]() {
    Serial.println("[WIFI] WLAN gespeichert – Neustart");
    user_apBlink(false);
  });

  bool ok = wm.autoConnect(WIFI_AP_NAME);
  if (ok) {
    // Direkt verbunden – kein AP nötig
    Serial.printf("[WIFI] IP=%s\n", WiFi.localIP().toString().c_str());
    return;
  }

  // AP-Modus läuft – warte auf Konfiguration (mit Blink-Loop)
  Serial.println("[WIFI] Warte auf WLAN-Konfiguration im AP...");
  unsigned long blinkAt = 0;
  bool blinkState = true;
  while (WiFi.status() != WL_CONNECTED) {
    wm.process();                    // WiFiManager-Loop (non-blocking)
    unsigned long now = millis();
    if (now - blinkAt >= 800) {
      blinkAt = now;
      blinkState = !blinkState;
      user_apBlink(blinkState);
    }
    yield();
  }
  user_apBlink(false);
  Serial.printf("[WIFI] IP=%s\n", WiFi.localIP().toString().c_str());
}

// ---- setup() & loop() --------------------------------------
static int _lastH = -1, _lastM = -1;

void setup() {
  Serial.begin(115200);
  Serial.printf("\n[BOOT] %s\n", DEVICE_NAME);

  settingsLoad();

  user_setup();                      // LEDs zuerst – damit AP-Blink funktioniert
  user_onBrightnessChange(gBrightness);

  wifiSetup();
  ntpSetup();
  webifSetup();

  user_onTimeChange(gHour, gMinute);
  _lastH = gHour; _lastM = gMinute;
}

void loop() {
  webifLoop();
  ntpUpdate();

  int H = gPreviewEnabled ? gPreviewHour   : (int)gHour;
  int M = gPreviewEnabled ? gPreviewMinute : (int)gMinute;

  if (!gPreviewEnabled && (H != _lastH || M != _lastM)) {
    uint16_t mins = (uint16_t)(H * 60 + M);
    bool dim = gDimEnabled && (
      gDimStart <= gDimEnd
        ? (mins >= gDimStart && mins < gDimEnd)
        : (mins >= gDimStart || mins < gDimEnd)
    );
    user_onBrightnessChange(dim ? gDimLevel : gBrightness);
    user_onTimeChange(H, M);
    _lastH = H; _lastM = M;
  }

  user_loop();
}

#endif
