// ============================================================
//  custom.cpp  –  Eigene Uhr
//
//  Hier den eigenen Display-Code implementieren.
//  In config.h:  #define ACTIVE_CLOCK  CUSTOM
//
//  Zur Verfügung stehende globale Variablen (aus settings.h):
//    gBrightness   – aktuelle Helligkeit (0..255)
//    gDimEnabled   – Dimmen aktiv?
//    gDimLevel     – Dim-Helligkeit
//    gDimStart/End – Zeitfenster in Minuten seit 00:00
//
//  Zur Verfügung stehende globale Variablen (aus ntp.h):
//    gHour, gMinute, gSecond  – aktuelle Zeit (uint8_t)
// ============================================================
#include "config.h"
#if ACTIVE_CLOCK == CUSTOM

#include <Arduino.h>
#include "settings.h"

// ---- Eigene Hardware-Initialisierung -----------------------
void user_setup() {
  // Beispiel: pinMode(MY_PIN, OUTPUT);
  Serial.println("[USER] CUSTOM ready");
}

// ---- Wird bei jedem Minutenwechsel aufgerufen --------------
void user_onTimeChange(int h, int m) {
  // Beispiel: Display aktualisieren
  Serial.printf("[USER] %02d:%02d\n", h, m);
}

// ---- Wird bei Helligkeitsänderung aufgerufen ---------------
void user_onBrightnessChange(uint8_t b) {
  // Beispiel: analogWrite(MY_PIN, b);
  (void)b;
}

// ---- Optionaler Loop-Code ----------------------------------
void user_loop() {
  // Wird in loop() aufgerufen – für zeitkritische Logik
}

// ---- AP-Modus Blink ----------------------------------------
void user_apBlink(bool on) {
  // "on" wechselt alle 600ms – Display AP-Hinweis anzeigen
  (void)on;
}
bool user_ledOn(int idx) { (void)idx; return false; }

void user_onColorsChange() {}
String user_getActiveLeds() { return "[]"; }
void user_allLedsOn() {}
void user_allLedsOff() {}
String user_timeToWords(int h, int m) { char b[12]; snprintf(b,12,"%02d:%02d",h,m); return String(b); }
#endif // CUSTOM
