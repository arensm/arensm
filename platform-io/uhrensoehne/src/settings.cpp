#include "settings.h"
#include "config.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

uint8_t  gBrightness   = BRIGHTNESS_DEFAULT;
bool     gDimEnabled   = DIM_ENABLED_DEFAULT;
uint8_t  gDimLevel     = DIM_LEVEL_DEFAULT;
uint16_t gDimStart     = DIM_START_DEFAULT;
uint16_t gDimEnd       = DIM_END_DEFAULT;

bool     gPreviewEnabled = false;
uint8_t  gPreviewHour    = 12;
uint8_t  gPreviewMinute  = 0;
uint8_t  gPreviewDay     = 1;
uint8_t  gPreviewMonth   = 1;
uint16_t gPreviewYear    = 1985;

// Farb-Defaults: Wort=weiß, Rahmen=dunkelgrün
uint8_t  gColorTimeR  = 255, gColorTimeG  = 255, gColorTimeB  = 255;
uint8_t  gColorFrameR =   0, gColorFrameG =  40, gColorFrameB =   5;

// 7-Segment: Sekundentakt der Punkte (Standard: an)
bool     gDotsEnabled = true;
bool     g12hMode     = false; // BTTF1: 12h/24h

// BTTF3: Destination Time – Default: 21. Okt. 2015, 16:29
uint8_t  gBttf3DestMon  = 10;
uint8_t  gBttf3DestDay  = 21;
uint16_t gBttf3DestYear = 2015;
uint8_t  gBttf3DestHour = 16;
uint8_t  gBttf3DestMin  = 29;

// BTTF3: Last Time Departed – Default: 26. Okt. 1985, 01:22
uint8_t  gBttf3LastMon  = 10;
uint8_t  gBttf3LastDay  = 26;
uint16_t gBttf3LastYear = 1985;
uint8_t  gBttf3LastHour =  1;
uint8_t  gBttf3LastMin  = 22;

// Debug-Passwort (Default aus config.h)
String   gDebugPassword = DEBUG_PASSWORD_DEFAULT;

// ---- Speichern ---------------------------------------------
void settingsSave() {
  File f = LittleFS.open(SETTINGS_PATH, "w");
  if (!f) { Serial.println("[CFG] Fehler: Datei nicht schreibbar"); return; }
  StaticJsonDocument<896> doc;
  doc["brightness"]   = gBrightness;
  doc["dimEnabled"]   = gDimEnabled;
  doc["dimLevel"]     = gDimLevel;
  doc["dimStart"]     = gDimStart;
  doc["dimEnd"]       = gDimEnd;
  doc["cTimeR"]       = gColorTimeR;
  doc["cTimeG"]       = gColorTimeG;
  doc["cTimeB"]       = gColorTimeB;
  doc["cFrameR"]      = gColorFrameR;
  doc["cFrameG"]      = gColorFrameG;
  doc["cFrameB"]      = gColorFrameB;
  doc["dotsEnabled"]  = gDotsEnabled;
  doc["mode12h"]      = g12hMode;
  doc["destMon"]      = gBttf3DestMon;
  doc["destDay"]      = gBttf3DestDay;
  doc["destYear"]     = gBttf3DestYear;
  doc["destHour"]     = gBttf3DestHour;
  doc["destMin"]      = gBttf3DestMin;
  doc["lastMon"]      = gBttf3LastMon;
  doc["lastDay"]      = gBttf3LastDay;
  doc["lastYear"]     = gBttf3LastYear;
  doc["lastHour"]     = gBttf3LastHour;
  doc["lastMin"]      = gBttf3LastMin;
  doc["debugPw"]      = gDebugPassword;
  serializeJson(doc, f);
  f.close();
  Serial.println("[CFG] Gespeichert");
}

// ---- Laden -------------------------------------------------
void settingsLoad() {
  if (!LittleFS.begin()) {
    Serial.println("[CFG] LittleFS mount fehlgeschlagen – formatiere...");
    LittleFS.format(); LittleFS.begin();
  }
  if (!LittleFS.exists(SETTINGS_PATH)) {
    Serial.println("[CFG] Keine settings.json – schreibe Defaults");
    settingsSave(); return;
  }
  File f = LittleFS.open(SETTINGS_PATH, "r");
  if (!f) { Serial.println("[CFG] Fehler: Datei nicht lesbar"); return; }
  StaticJsonDocument<896> doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) {
    Serial.printf("[CFG] JSON-Fehler: %s – schreibe Defaults\n", err.c_str());
    settingsSave(); return;
  }
  if (doc.containsKey("brightness"))  gBrightness  = (uint8_t)doc["brightness"].as<int>();
  if (doc.containsKey("dimEnabled"))  gDimEnabled  = doc["dimEnabled"].as<bool>();
  if (doc.containsKey("dimLevel"))    gDimLevel    = (uint8_t)doc["dimLevel"].as<int>();
  if (doc.containsKey("dimStart"))    gDimStart    = (uint16_t)doc["dimStart"].as<int>();
  if (doc.containsKey("dimEnd"))      gDimEnd      = (uint16_t)doc["dimEnd"].as<int>();
  if (doc.containsKey("cTimeR"))      gColorTimeR  = (uint8_t)doc["cTimeR"].as<int>();
  if (doc.containsKey("cTimeG"))      gColorTimeG  = (uint8_t)doc["cTimeG"].as<int>();
  if (doc.containsKey("cTimeB"))      gColorTimeB  = (uint8_t)doc["cTimeB"].as<int>();
  if (doc.containsKey("cFrameR"))     gColorFrameR = (uint8_t)doc["cFrameR"].as<int>();
  if (doc.containsKey("cFrameG"))     gColorFrameG = (uint8_t)doc["cFrameG"].as<int>();
  if (doc.containsKey("cFrameB"))     gColorFrameB = (uint8_t)doc["cFrameB"].as<int>();
  if (doc.containsKey("dotsEnabled")) gDotsEnabled = doc["dotsEnabled"].as<bool>();
  if (doc.containsKey("mode12h"))     g12hMode        = doc["mode12h"].as<bool>();
  if (doc.containsKey("destMon"))     gBttf3DestMon   = (uint8_t)doc["destMon"].as<int>();
  if (doc.containsKey("destDay"))     gBttf3DestDay   = (uint8_t)doc["destDay"].as<int>();
  if (doc.containsKey("destYear"))    gBttf3DestYear  = (uint16_t)doc["destYear"].as<int>();
  if (doc.containsKey("destHour"))    gBttf3DestHour  = (uint8_t)doc["destHour"].as<int>();
  if (doc.containsKey("destMin"))     gBttf3DestMin   = (uint8_t)doc["destMin"].as<int>();
  if (doc.containsKey("lastMon"))     gBttf3LastMon   = (uint8_t)doc["lastMon"].as<int>();
  if (doc.containsKey("lastDay"))     gBttf3LastDay   = (uint8_t)doc["lastDay"].as<int>();
  if (doc.containsKey("lastYear"))    gBttf3LastYear  = (uint16_t)doc["lastYear"].as<int>();
  if (doc.containsKey("lastHour"))    gBttf3LastHour  = (uint8_t)doc["lastHour"].as<int>();
  if (doc.containsKey("lastMin"))     gBttf3LastMin   = (uint8_t)doc["lastMin"].as<int>();
  if (doc.containsKey("debugPw"))     gDebugPassword = doc["debugPw"].as<String>();
  Serial.printf("[CFG] Geladen: bri=%u dim=%s %u-%u lvl=%u\n",
    gBrightness, gDimEnabled?"an":"aus", gDimStart, gDimEnd, gDimLevel);
}

// ---- Shortcuts ---------------------------------------------
void settingsSaveBrightness(uint8_t b) { gBrightness = b; settingsSave(); }
void settingsSaveDim()                  { settingsSave(); }
void settingsSaveColors()               { settingsSave(); }
