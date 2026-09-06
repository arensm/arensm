// ============================================================
//  wordclock.cpp  –  WordClock ARI
//  NeoPixel / WS2812B, 10×11 Buchstaben + 4 Minutenpunkte
//  = 114 LEDs, Serpentine-Verkabelung
//
//  Pin (in config.h):
//    PIN_NEOPIXEL = D4
//
//  Textmatrix:
//    0: ES_IST_FÜNF   1: ZEHN_ZWANZIG  2: DREI_VIERTEL
//    3: AN_NACH_M_VOR 4: HALB_M_ZWÖLF  5: ZWEI_N_SIEBEN
//    6: DREI_ARI_FÜNF 7: ELF_NEUN_VIER 8: MACHT_A_ZEHN
//    9: SECHS_ARI_UHR + 4 Minutenpunkte (Index 110–113)
// ============================================================
#include "config.h"
#if ACTIVE_CLOCK == WORDCLOCK

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "settings.h"
#include "ntp.h"

#define WC_COLS 11

static Adafruit_NeoPixel strip(NEO_COUNT, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
static const uint16_t DOTS[4] = {110, 111, 112, 113};

// ---- Koordinaten → LED-Index (Serpentine) ------------------
static uint16_t wc_idx(uint8_t row, uint8_t col) {
  return (row % 2 == 0) ? row * WC_COLS + col
                        : row * WC_COLS + (WC_COLS - 1 - col);
}
static void wc_setWord(uint8_t row, uint8_t startCol, uint8_t len, uint32_t c) {
  for (uint8_t i = 0; i < len; i++)
    strip.setPixelColor(wc_idx(row, startCol + i), c);
}
static void wc_clear() {
  for (uint16_t i = 0; i < NEO_COUNT; i++) strip.setPixelColor(i, 0);
}
static void wc_dots(uint8_t n, uint32_t c) {
  for (uint8_t i = 0; i < 4; i++) strip.setPixelColor(DOTS[i], (i < n) ? c : 0);
}
static uint32_t WC_COL() { return strip.Color(gColorTimeR, gColorTimeG, gColorTimeB); }

// ---- Wort-Positionen ----------------------------------------
static void w_ES(uint32_t c)      { wc_setWord(0, 0, 2, c); }
static void w_IST(uint32_t c)     { wc_setWord(0, 3, 3, c); }
static void w_FUENF_m(uint32_t c) { wc_setWord(0, 7, 4, c); }
static void w_ZEHN_m(uint32_t c)  { wc_setWord(1, 0, 4, c); }
static void w_ZWANZIG(uint32_t c) { wc_setWord(1, 4, 7, c); }
static void w_VIERTEL(uint32_t c) { wc_setWord(2, 4, 7, c); }
static void w_NACH(uint32_t c)    { wc_setWord(3, 2, 4, c); }
static void w_VOR(uint32_t c)     { wc_setWord(3, 7, 3, c); }
static void w_HALB(uint32_t c)    { wc_setWord(4, 0, 4, c); }
static void w_UHR(uint32_t c)     { wc_setWord(9, 8, 3, c); }
// Stunden (h==1 hat zwei Varianten)
static void w_EIN(uint32_t c)     { wc_setWord(5, 2, 3, c); }   // "EIN Uhr"
static void w_EINS(uint32_t c)    { wc_setWord(5, 2, 4, c); }   // "EINS" z.B. "fuenf nach EINS"
static void w_ZWEI(uint32_t c)    { wc_setWord(5, 0, 4, c); }
static void w_DREI(uint32_t c)    { wc_setWord(6, 0, 4, c); }
static void w_VIER(uint32_t c)    { wc_setWord(7, 7, 4, c); }
static void w_FUENF_h(uint32_t c) { wc_setWord(6, 7, 4, c); }
static void w_SECHS(uint32_t c)   { wc_setWord(9, 0, 5, c); }
static void w_SIEBEN(uint32_t c)  { wc_setWord(5, 5, 6, c); }
static void w_ACHT(uint32_t c)    { wc_setWord(8, 1, 4, c); }
static void w_NEUN(uint32_t c)    { wc_setWord(7, 3, 4, c); }
static void w_ZEHN_h(uint32_t c)  { wc_setWord(8, 6, 4, c); }
static void w_ELF(uint32_t c)     { wc_setWord(7, 0, 3, c); }
static void w_ZWOELF(uint32_t c)  { wc_setWord(4, 5, 5, c); }

// ---- Stundenwort auswählen ---------------------------------
static int wc_dispHour(int h24, int m5) {
  int h = h24 % 12; if (h == 0) h = 12;
  if (m5 >= 25) { h = (h % 12) + 1; if (h == 0) h = 12; }
  return h;
}
static void wc_hourWord(int h, bool exactHour, uint32_t c) {
  if (h == 1) { if (exactHour) w_EIN(c); else w_EINS(c); return; }
  switch (h) {
    case 2:  w_ZWEI(c);   break; case 3:  w_DREI(c);   break;
    case 4:  w_VIER(c);   break; case 5:  w_FUENF_h(c);break;
    case 6:  w_SECHS(c);  break; case 7:  w_SIEBEN(c); break;
    case 8:  w_ACHT(c);   break; case 9:  w_NEUN(c);   break;
    case 10: w_ZEHN_h(c); break; case 11: w_ELF(c);    break;
    default: w_ZWOELF(c); break;
  }
}

// ---- Zeitanzeige rendern -----------------------------------
static void wc_render(int h24, int m) {
  wc_clear();
  int m5 = m / 5 * 5, rest = m % 5;
  int hW = wc_dispHour(h24, m5);
  uint32_t c = WC_COL();
  w_ES(c); w_IST(c);
  switch (m5) {
    case 0:  wc_hourWord(hW, true,  c); w_UHR(c); break;
    case 5:  w_FUENF_m(c); w_NACH(c);  wc_hourWord(hW, false, c); break;
    case 10: w_ZEHN_m(c);  w_NACH(c);  wc_hourWord(hW, false, c); break;
    case 15: w_VIERTEL(c); w_NACH(c);  wc_hourWord(hW, false, c); break;
    case 20: w_ZWANZIG(c); w_NACH(c);  wc_hourWord(hW, false, c); break;
    case 25: w_FUENF_m(c); w_VOR(c);   w_HALB(c); wc_hourWord(hW, false, c); break;
    case 30: w_HALB(c);    wc_hourWord(hW, false, c); break;
    case 35: w_FUENF_m(c); w_NACH(c);  w_HALB(c); wc_hourWord(hW, false, c); break;
    case 40: w_ZWANZIG(c); w_VOR(c);   wc_hourWord(hW, false, c); break;
    case 45: w_VIERTEL(c); w_VOR(c);   wc_hourWord(hW, false, c); break;
    case 50: w_ZEHN_m(c);  w_VOR(c);   wc_hourWord(hW, false, c); break;
    case 55: w_FUENF_m(c); w_VOR(c);   wc_hourWord(hW, false, c); break;
  }
  wc_dots(rest, c);
  strip.setBrightness(gBrightness);
  strip.show();
}

// ---- Uhrzeit als Satz (für WebIF + Serial) ------------------
static bool wc_initialized = false;
static String wc_hourName(int h, bool exactHour) {
  if (h == 1) return exactHour ? "ein" : "eins";
  const char* names[] = {"","ein","zwei","drei","vier","fuenf","sechs",
                          "sieben","acht","neun","zehn","elf","zwoelf"};
  return String(names[h]);
}

String user_timeToWords(int h24, int m) {
  int m5   = m / 5 * 5;
  int rest = m % 5;
  int hW   = wc_dispHour(h24, m5);
  // Striche für Minutenpunkte (wie auf dem Display)
  String dots = "";
  for (int i = 0; i < rest; i++) dots += "-";
  if (rest > 0) dots = "  " + dots;

  String s = "Es ist ";
  switch (m5) {
    case 0:  s += wc_hourName(hW, true)   + " uhr";              break;
    case 5:  s += "fuenf nach "     + wc_hourName(hW, false);     break;
    case 10: s += "zehn nach "     + wc_hourName(hW, false);     break;
    case 15: s += "viertel nach "  + wc_hourName(hW, false);     break;
    case 20: s += "zwanzig nach "  + wc_hourName(hW, false);     break;
    case 25: s += "fuenf vor halb " + wc_hourName(hW, false);     break;
    case 30: s += "halb "          + wc_hourName(hW, false);     break;
    case 35: s += "fuenf nach halb "+ wc_hourName(hW, false);     break;
    case 40: s += "zwanzig vor "   + wc_hourName(hW, false);     break;
    case 45: s += "viertel vor "   + wc_hourName(hW, false);     break;
    case 50: s += "zehn vor "      + wc_hourName(hW, false);     break;
    case 55: s += "fuenf vor "      + wc_hourName(hW, false);     break;
    default: s += String(h24) + ":" + (m < 10 ? "0" : "") + String(m); break;
  }
  return s + dots;
}

// ---- Hook-Funktionen ----------------------------------------
void user_setup() {
  strip.begin();
  strip.setBrightness(gBrightness);
  strip.show();
  wc_initialized = true;
  Serial.println("[USER] WORDCLOCK ready");
}
void user_onTimeChange(int h, int m) {
  if (!wc_initialized) return;
  wc_render(h, m);
  Serial.printf("[USER] %02d:%02d – %s\n", h, m, user_timeToWords(h, m).c_str());
}
void user_onBrightnessChange(uint8_t b) {
  if (!wc_initialized) return;
  strip.setBrightness(b);
  strip.show();
}
void user_loop() {}
void user_apBlink(bool on) {
  if (!wc_initialized) return;
  wc_clear();
  if (on) {
    uint32_t c = strip.Color(255, 140, 0);  // Orange für AP-Modus
    wc_setWord(6, 4, 3, c);  // ARI (Zeile 6, Sp. 4–6)
    wc_setWord(9, 8, 3, c);  // UHR (Zeile 9, Sp. 8–10)
  }
  strip.show();
}
void user_allLedsOff() {
  if (!wc_initialized) return;
  strip.clear();
  strip.show();
}
void user_allLedsOn() {
  if (!wc_initialized) return;
  for (int i = 0; i < NEO_COUNT; i++) strip.setPixelColor(i, strip.Color(255, 255, 255));
  strip.setBrightness(255);
  strip.show();
}
bool user_ledOn(int idx) {
  if (!wc_initialized) return false;
  for (int i = 0; i < NEO_COUNT; i++) strip.setPixelColor(i, 0);
  if (idx >= 0 && idx < NEO_COUNT) strip.setPixelColor(idx, strip.Color(255, 255, 255));
  strip.setBrightness(255);
  strip.show();
  return true;
}

void user_onColorsChange() {
  if (!wc_initialized) return;
  uint8_t h = gPreviewEnabled ? gPreviewHour   : gHour;
  uint8_t m = gPreviewEnabled ? gPreviewMinute : gMinute;
  wc_render(h, m);
}
String user_getActiveLeds() {
  if (!wc_initialized) return "[]";
  String s = "[";
  bool first = true;
  for (int i = 0; i < NEO_COUNT; i++) {
    if (strip.getPixelColor(i) != 0) {
      if (!first) s += ",";
      s += i;
      first = false;
    }
  }
  return s + "]";
}
#endif // WORDCLOCK
