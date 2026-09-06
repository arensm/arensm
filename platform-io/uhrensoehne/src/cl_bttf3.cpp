// ============================================================
//  cl_bttf3.cpp  –  BTTF-3 Clock  (ESP32)
//
//  3 Zeilen mit je 3 TM1637-Displays (Month/Day | Year | Hour/Min)
//  + AM/PM-LEDs pro Zeile
//
//  Zeile 1 – Destination Time  (rot,   feste Zeit via WebIF)
//  Zeile 2 – Present Time      (grün,  NTP-Zeit)
//  Zeile 3 – Last Time Departed (gelb, feste Zeit via WebIF)
//
//  Pins → config.h  (BTTF3_R_*, BTTF3_G_*, BTTF3_Y_*)
//  In config.h:  #define ACTIVE_CLOCK  BTTF3
// ============================================================
#include "config.h"
#if ACTIVE_CLOCK == BTTF3

#include <Arduino.h>
#include <TM1637Display.h>
#include "settings.h"
#include "ntp.h"

// ---- Display-Objekte ---------------------------------------
// Zeile 1: Destination Time (rot)
static TM1637Display rMD(BTTF3_R_CLK, BTTF3_R_DIO_MD);
static TM1637Display rYR(BTTF3_R_CLK, BTTF3_R_DIO_YR);
static TM1637Display rHM(BTTF3_R_CLK, BTTF3_R_DIO_HM);
// Zeile 2: Present Time (grün)
static TM1637Display gMD(BTTF3_G_CLK, BTTF3_G_DIO_MD);
static TM1637Display gYR(BTTF3_G_CLK, BTTF3_G_DIO_YR);
static TM1637Display gHM(BTTF3_G_CLK, BTTF3_G_DIO_HM);
// Zeile 3: Last Time Departed (gelb)
static TM1637Display yMD(BTTF3_Y_CLK, BTTF3_Y_DIO_MD);
static TM1637Display yYR(BTTF3_Y_CLK, BTTF3_Y_DIO_YR);
static TM1637Display yHM(BTTF3_Y_CLK, BTTF3_Y_DIO_HM);

// ---- Feste Zeiten (Destination + Last Departed) ------------
// Werden über WebIF gesetzt und in LittleFS gespeichert
// (gBttf3Dest* / gBttf3Last* in settings.h)

// ---- Hilfsfunktionen ---------------------------------------
static void setRowBrightness(TM1637Display &d1, TM1637Display &d2,
                              TM1637Display &d3, uint8_t b) {
  uint8_t b7 = map(b, 0, 255, 0, 7);
  d1.setBrightness(b7);
  d2.setBrightness(b7);
  d3.setBrightness(b7);
}

static void showRow(TM1637Display &dMD, TM1637Display &dYR,
                    TM1637Display &dHM,
                    int mon, int day, int year, int h, int m,
                    int pinAM, int pinPM) {
  dMD.showNumberDecEx(mon,  0b01000000, true, 2, 0);
  dMD.showNumberDecEx(day,  0b01000000, true, 2, 2);
  dYR.showNumberDecEx(year, 0b00000000, true);
  // 12h-Konvertierung falls aktiv
  int hDisp = h;
  if (g12hMode) { hDisp = h % 12; if (hDisp == 0) hDisp = 12; }
  dHM.showNumberDecEx(hDisp, 0b01000000, true, 2, 0);
  dHM.showNumberDecEx(m,     0b01000000, true, 2, 2);
  // AM/PM LEDs
  bool isPM = (h >= 12);
  if (pinAM >= 0) digitalWrite(pinAM, (g12hMode && !isPM) ? HIGH : LOW);
  if (pinPM >= 0) digitalWrite(pinPM, (g12hMode &&  isPM) ? HIGH : LOW);
}

static void clearRow(TM1637Display &d1, TM1637Display &d2,
                     TM1637Display &d3, int pinAM, int pinPM) {
  d1.clear(); d2.clear(); d3.clear();
  if (pinAM >= 0) digitalWrite(pinAM, LOW);
  if (pinPM >= 0) digitalWrite(pinPM, LOW);
}

static void initPins(int pinAM, int pinPM) {
  if (pinAM >= 0) { pinMode(pinAM, OUTPUT); digitalWrite(pinAM, LOW); }
  if (pinPM >= 0) { pinMode(pinPM, OUTPUT); digitalWrite(pinPM, LOW); }
}

// ============================================================
//  Hook-Implementierungen
// ============================================================

void user_setup() {
  uint8_t b7 = map(gBrightness, 0, 255, 0, 7);

  setRowBrightness(rMD, rYR, rHM, gBrightness);
  setRowBrightness(gMD, gYR, gHM, gBrightness);
  setRowBrightness(yMD, yYR, yHM, gBrightness);

  clearRow(rMD, rYR, rHM, BTTF3_R_PIN_AM, BTTF3_R_PIN_PM);
  clearRow(gMD, gYR, gHM, BTTF3_G_PIN_AM, BTTF3_G_PIN_PM);
  clearRow(yMD, yYR, yHM, BTTF3_Y_PIN_AM, BTTF3_Y_PIN_PM);

  initPins(BTTF3_R_PIN_AM, BTTF3_R_PIN_PM);
  initPins(BTTF3_G_PIN_AM, BTTF3_G_PIN_PM);
  initPins(BTTF3_Y_PIN_AM, BTTF3_Y_PIN_PM);

  Serial.println("[USER] BTTF-3 ready");
}

void user_onTimeChange(int h, int m) {
  // Zeile 1: Destination Time (feste Werte aus Settings)
  showRow(rMD, rYR, rHM,
          gBttf3DestMon, gBttf3DestDay, gBttf3DestYear,
          gBttf3DestHour, gBttf3DestMin,
          BTTF3_R_PIN_AM, BTTF3_R_PIN_PM);

  // Zeile 2: Present Time (NTP)
  time_t now = time(nullptr);
  struct tm *t = localtime(&now);
  showRow(gMD, gYR, gHM,
          t->tm_mon + 1, t->tm_mday, t->tm_year + 1900,
          h, m,
          BTTF3_G_PIN_AM, BTTF3_G_PIN_PM);

  // Zeile 3: Last Time Departed (feste Werte aus Settings)
  showRow(yMD, yYR, yHM,
          gBttf3LastMon, gBttf3LastDay, gBttf3LastYear,
          gBttf3LastHour, gBttf3LastMin,
          BTTF3_Y_PIN_AM, BTTF3_Y_PIN_PM);

  Serial.printf("[BTTF3] Present: %02d:%02d\n", h, m);
}

void user_onBrightnessChange(uint8_t b) {
  setRowBrightness(rMD, rYR, rHM, b);
  setRowBrightness(gMD, gYR, gHM, b);
  setRowBrightness(yMD, yYR, yHM, b);
  // Neu schreiben damit Helligkeit sofort wirkt
  user_onTimeChange(gPreviewEnabled ? gPreviewHour : (int)gHour,
                    gPreviewEnabled ? gPreviewMinute : (int)gMinute);
}

void user_loop() {}

void user_apBlink(bool on) {
  const uint8_t SEG_DASH = 0b01000000;
  uint8_t seg[4] = { SEG_DASH, SEG_DASH, SEG_DASH, SEG_DASH };
  if (on) {
    rMD.setSegments(seg); rYR.setSegments(seg); rHM.setSegments(seg);
    gMD.setSegments(seg); gYR.setSegments(seg); gHM.setSegments(seg);
    yMD.setSegments(seg); yYR.setSegments(seg); yHM.setSegments(seg);
  } else {
    clearRow(rMD, rYR, rHM, BTTF3_R_PIN_AM, BTTF3_R_PIN_PM);
    clearRow(gMD, gYR, gHM, BTTF3_G_PIN_AM, BTTF3_G_PIN_PM);
    clearRow(yMD, yYR, yHM, BTTF3_Y_PIN_AM, BTTF3_Y_PIN_PM);
  }
}

void user_allLedsOn() {
  uint8_t all[4] = {0xFF, 0xFF, 0xFF, 0xFF};
  rMD.setSegments(all); rYR.setSegments(all); rHM.setSegments(all);
  gMD.setSegments(all); gYR.setSegments(all); gHM.setSegments(all);
  yMD.setSegments(all); yYR.setSegments(all); yHM.setSegments(all);
}

void user_allLedsOff() {
  clearRow(rMD, rYR, rHM, BTTF3_R_PIN_AM, BTTF3_R_PIN_PM);
  clearRow(gMD, gYR, gHM, BTTF3_G_PIN_AM, BTTF3_G_PIN_PM);
  clearRow(yMD, yYR, yHM, BTTF3_Y_PIN_AM, BTTF3_Y_PIN_PM);
}

bool user_ledOn(int idx)   { (void)idx; return false; }
void user_onColorsChange() {}
String user_getActiveLeds(){ return "[]"; }

String user_timeToWords(int h, int m) {
  time_t now = time(nullptr);
  struct tm *t = localtime(&now);
  char buf[24];
  snprintf(buf, sizeof(buf), "%02d.%02d.%04d %02d:%02d",
    t->tm_mday, t->tm_mon+1, t->tm_year+1900, h, m);
  return String(buf);
}

#endif // BTTF3
