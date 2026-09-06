#pragma once
#include <Arduino.h>
// ============================================================
//  settings.h  –  LittleFS/JSON persistente Einstellungen
// ============================================================

extern uint8_t  gBrightness;
extern bool     gDimEnabled;
extern uint8_t  gDimLevel;
extern uint16_t gDimStart;
extern uint16_t gDimEnd;

extern bool     gPreviewEnabled;
extern uint8_t  gPreviewHour;
extern uint8_t  gPreviewMinute;
extern uint8_t  gPreviewDay;    // nur BTTF1: Test-Tag
extern uint8_t  gPreviewMonth;  // nur BTTF1: Test-Monat
extern uint16_t gPreviewYear;   // nur BTTF1: Test-Jahr

// Farben (FUTURE_WC: Wortfarbe + Rahmenfarbe)
extern uint8_t  gColorTimeR, gColorTimeG, gColorTimeB;
extern uint8_t  gColorFrameR, gColorFrameG, gColorFrameB;

// 7-Segment: Sekundentakt der Punkte
extern bool     gDotsEnabled;

// BTTF1: 12h-Modus (AM/PM aktiv)
extern bool     g12hMode;

// BTTF3: Destination Time (feste Zeit, Zeile 1)
extern uint8_t  gBttf3DestMon, gBttf3DestDay;
extern uint16_t gBttf3DestYear;
extern uint8_t  gBttf3DestHour, gBttf3DestMin;

// BTTF3: Last Time Departed (feste Zeit, Zeile 3)
extern uint8_t  gBttf3LastMon, gBttf3LastDay;
extern uint16_t gBttf3LastYear;
extern uint8_t  gBttf3LastHour, gBttf3LastMin;

// Debug-Passwort (LittleFS-persistent, initial aus der lokalen .env)
extern String   gDebugPassword;

void settingsLoad();
void settingsSave();
void settingsSaveBrightness(uint8_t b);
void settingsSaveDim();
void settingsSaveColors();
