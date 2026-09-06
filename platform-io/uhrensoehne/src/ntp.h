#pragma once
#include <Arduino.h>
// ============================================================
//  ntp.h  –  Zeitverwaltung (NTP + millis-Fallback)
// ============================================================

extern uint8_t  gHour;
extern uint8_t  gMinute;
extern uint8_t  gSecond;

// Einmalig beim Start aufrufen
void ntpSetup();

// In loop() aufrufen
void ntpUpdate();

// Manuellen NTP-Sync erzwingen (z.B. über WebIF)
void ntpForceSync();

// Hilfsfunktion: liefert Minuten seit 00:00 der aktuellen Anzeigezeit
uint16_t currentMinutesSinceMidnight();

// Uptime als "0d 00:01:23"
String uptimeString();

// "HH:MM" → Minuten seit 00:00, false bei Fehler
bool parseHHMM(const String& s, uint16_t& out);

// Minuten seit 00:00 → "HH:MM"
String formatHHMM(uint16_t mins);
