// ============================================================
//  clock_7segmente.cpp  –  7-SEGMENT-CLOCK
//  NeoPixel / WS2812B, 30 LEDs (Index 0–29), Pin D4
//
//  Layout (Front view, IDs 0-basiert):
//
//  Digit 3 (Std-Zehner)    Digit 2 (Std-Einer)   :   Digit 1 (Min-Zehner)   Digit 0 (Min-Einer)
//       [23]                    [16]             [28]       [9]                   [2]
//   [24]   [22]            [17]    [15]                 [10]  [8]             [3]   [1]
//       [25]                    [18]                        [11]                  [4]
//   [26]   [21]            [19]    [14]          [29]  [12]   [7]             [5]   [0]
//       [27]                    [20]                        [13]                  [6]
//
//  Segment-Mapping pro Digit (a=oben, b=r-oben, c=r-unten, d=unten, e=l-unten, f=l-oben, g=mitte):
//    Digit 0 (Min-Einer):  a=2,  b=1,  c=0,  d=6,  e=5,  f=3,  g=4
//    Digit 1 (Min-Zehner): a=9,  b=8,  c=7,  d=13, e=12, f=10, g=11
//    Digit 2 (Std-Einer):  a=16, b=15, c=14, d=20, e=19, f=17, g=18
//    Digit 3 (Std-Zehner): a=23, b=22, c=21, d=27, e=26, f=24, g=25
//    Doppelpunkt:          oben=28, unten=29
// ============================================================
#include "config.h"
#if ACTIVE_CLOCK == SIEBEN_SEGMENTE

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "settings.h"
#include "ntp.h"

#define SEG_NUM_LEDS  30

static Adafruit_NeoPixel strip(SEG_NUM_LEDS, SEG_DATA_PIN, NEO_GRB + NEO_KHZ800);
static bool seg_initialized = false;

// ---- Segment-Definitionen (a–g + Doppelpunkt) ---------------
// Jede Ziffer: {a, b, c, d, e, f, g}
static const uint8_t SEG_DIGITS[4][7] = {
  { 2,  1,  0,  6,  5,  3,  4},  // Digit 0: Minuten-Einer
  { 9,  8,  7, 13, 12, 10, 11},  // Digit 1: Minuten-Zehner
  {16, 15, 14, 20, 19, 17, 18},  // Digit 2: Stunden-Einer
  {23, 22, 21, 27, 26, 24, 25},  // Digit 3: Stunden-Zehner
};
static const uint8_t SEG_COLON[2] = {28, 29};  // Doppelpunkt oben/unten

// 7-Segment-Enkodierung: Bit 0=a, 1=b, 2=c, 3=d, 4=e, 5=f, 6=g
//                         a    b    c    d    e    f    g
static const uint8_t SEG_FONT[10] = {
  0b0111111,  // 0: a,b,c,d,e,f
  0b0000110,  // 1: b,c
  0b1011011,  // 2: a,b,d,e,g
  0b1001111,  // 3: a,b,c,d,g
  0b1100110,  // 4: b,c,f,g
  0b1101101,  // 5: a,c,d,f,g
  0b1111101,  // 6: a,c,d,e,f,g
  0b0000111,  // 7: a,b,c
  0b1111111,  // 8: alle
  0b1101111,  // 9: a,b,c,d,f,g
};

#define SEG_COL() strip.Color(gColorTimeR, gColorTimeG, gColorTimeB)

// ---- Hilfsfunktionen ----------------------------------------
static void seg_clear() {
  strip.clear();
}

static void seg_drawDigit(uint8_t pos, uint8_t digit, uint32_t col) {
  if (pos > 3 || digit > 9) return;
  uint8_t enc = SEG_FONT[digit];
  for (uint8_t seg = 0; seg < 7; seg++) {
    if (enc & (1 << seg))
      strip.setPixelColor(SEG_DIGITS[pos][seg], col);
  }
}

static void seg_render(int h, int m) {
  seg_clear();
  uint32_t c = SEG_COL();
  seg_drawDigit(0, m % 10,        c);   // Minuten-Einer
  seg_drawDigit(1, (m / 10) % 10, c);   // Minuten-Zehner
  seg_drawDigit(2, h % 10,        c);   // Stunden-Einer
  seg_drawDigit(3, (h / 10) % 10, c);   // Stunden-Zehner
  // Doppelpunkt: immer an, oder Sekundentakt
  if (gDotsEnabled) {
    bool dotsOn = (millis() / 1000) % 2 == 0;
    if (dotsOn) {
      strip.setPixelColor(SEG_COLON[0], c);
      strip.setPixelColor(SEG_COLON[1], c);
    }
  } else {
    strip.setPixelColor(SEG_COLON[0], c);
    strip.setPixelColor(SEG_COLON[1], c);
  }
  strip.setBrightness(gBrightness);
  strip.show();
}

// Letzter Sekundenzustand für Blink-Loop
static bool _lastDotState = true;

// ---- Hook-Implementierungen ---------------------------------
void user_setup() {
  strip.begin();
  strip.setBrightness(gBrightness);
  strip.clear();
  strip.show();
  seg_initialized = true;
  Serial.println("[USER] SIEBEN-SEGMENTE ready");
}

void user_onTimeChange(int h, int m) {
  if (!seg_initialized) return;
  seg_render(h, m);
  Serial.printf("[USER] %02d:%02d\n", h, m);
}

void user_onBrightnessChange(uint8_t b) {
  if (!seg_initialized) return;
  strip.setBrightness(b);
  strip.show();
}

void user_loop() {
  if (!seg_initialized || !gDotsEnabled) return;
  // Sekundentakt: Doppelpunkt jede Sekunde toggling
  bool dotOn = (millis() / 1000) % 2 == 0;
  if (dotOn != _lastDotState) {
    _lastDotState = dotOn;
    uint32_t c = dotOn ? SEG_COL() : 0;
    strip.setPixelColor(SEG_COLON[0], c);
    strip.setPixelColor(SEG_COLON[1], c);
    strip.show();
  }
}

void user_apBlink(bool on) {
  if (!seg_initialized) return;
  seg_clear();
  if (on) {
    uint32_t c = strip.Color(255, 140, 0);  // Orange
    // "A" auf Digit 1 (Min-Zehner): Segmente a,b,c,e,f,g (kein d)
    for (uint8_t s : {0,1,2,4,5,6}) strip.setPixelColor(SEG_DIGITS[1][s], c);
    // "P" auf Digit 0 (Min-Einer): Segmente a,b,e,f,g (kein c,d)
    for (uint8_t s : {0,1,4,5,6}) strip.setPixelColor(SEG_DIGITS[0][s], c);
  }
  strip.setBrightness(255);
  strip.show();
}

bool user_ledOn(int idx) {
  if (!seg_initialized || idx < 0 || idx >= SEG_NUM_LEDS) return false;
  strip.clear();
  strip.setPixelColor(idx, strip.Color(255, 255, 255));
  strip.setBrightness(255);
  strip.show();
  return true;
}

void user_allLedsOn() {
  if (!seg_initialized) return;
  for (int i = 0; i < SEG_NUM_LEDS; i++)
    strip.setPixelColor(i, strip.Color(255, 255, 255));
  strip.setBrightness(255);
  strip.show();
}

void user_allLedsOff() {
  if (!seg_initialized) return;
  strip.clear();
  strip.show();
}

void user_onColorsChange() {
  if (!seg_initialized) return;
  uint8_t h = gPreviewEnabled ? gPreviewHour   : gHour;
  uint8_t m = gPreviewEnabled ? gPreviewMinute : gMinute;
  seg_render(h, m);
}

String user_getActiveLeds() {
  if (!seg_initialized) return "[]";
  String s = "[";
  bool first = true;
  for (int i = 0; i < SEG_NUM_LEDS; i++) {
    if (strip.getPixelColor(i) != 0) {
      if (!first) s += ",";
      s += i;
      first = false;
    }
  }
  return s + "]";
}

String user_timeToWords(int h, int m) {
  char b[6];
  snprintf(b, 6, "%02d:%02d", h, m);
  return String(b);
}

#endif // SIEBEN_SEGMENTE
