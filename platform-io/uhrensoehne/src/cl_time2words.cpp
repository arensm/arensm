// ============================================================
//  time2words.cpp  –  FUTURE-WORD-CLOCK
//  FastLED / WS2812B, 40 LEDs, Pin D8
//
//  LED-Layout (1-basiert, bestätigt):
//  1–3   = Frame oben (bungard ihr weg)
//  4     = ein
//  5–6   = viertel
//  7     = zehn (min)
//  8     = fünf (min)
//  9–10  = gleich
//  11–12 = zwanzig
//  13    = kurz
//  14    = nach
//  15    = vor
//  16    = vier   17 = drei   18 = zwei   19 = eins   20 = halb
//  21    = fünf(h)   22–23 = sechs   24–25 = sieben   26 = acht
//  27–28 = zwölf   29 = elf   30 = zehn(h)   31 = neun
//  32    = uhr   33–34 = mittags   35–36 = nachts
//  37–40 = Frame unten (zur leiterplatte)
// ============================================================
#include "config.h"
#if ACTIVE_CLOCK == TIME_2_WORDS

#include <Arduino.h>
#include <FastLED.h>
#include "settings.h"
#include "ntp.h"

static CRGB fwc_leds[FWC_NUM_LEDS];

// Farben direkt aus globalen Settings (kein Cache der veralten kann)
#define FWC_COLOR_TIME  CRGB(gColorTimeR,  gColorTimeG,  gColorTimeB)
#define FWC_COLOR_FRAME CRGB(gColorFrameR, gColorFrameG, gColorFrameB)

// ---- LED-IDs (1-basiert, bestätigt nach Frontplattenbild) --
//  Z1: ID1–3 = bungard ihr weg (Frame oben)   ID4 = ein
//  Z2 (rechts→links): ID5–6=viertel  ID7=zehn  ID8=fünf  ID9=gleich①  ID10=gleich②
//  Z3: ID11–12=zwanzig  ID13=kurz  ID14=nach  ID15=vor
//  Z4: ID16=vier  ID17=drei  ID18=zwei  ID19=eins  ID20=halb
//  Z5: ID21=fünf  ID22–23=sechs  ID24–25=sieben  ID26=acht
//  Z6: ID27–28=zwölf  ID29=elf  ID30=zehn  ID31=neun
//  Z7: ID32=uhr  ID33–34=mittags  ID35–36=nachts
//  Z8: ID37–39=leiterplatte (Frame unten)  ID40=zur
static const uint8_t W_GLEICH[]  = {9, 10};
static const uint8_t W_FUENF_M[] = {8};
static const uint8_t W_ZEHN_M[]  = {7};
static const uint8_t W_VIERTEL[] = {5, 6};
static const uint8_t W_ZWANZIG[] = {11, 12};
static const uint8_t W_KURZ[]    = {13};
static const uint8_t W_NACH[]    = {14};
static const uint8_t W_VOR[]     = {15};
static const uint8_t W_HALB[]    = {20};
static const uint8_t W_UHR[]     = {32};
static const uint8_t W_MITTAGS[] = {33, 34};
static const uint8_t W_NACHTS[]  = {35, 36};
static const uint8_t H_EIN[]     = {4};
static const uint8_t H_EINS[]    = {19};
static const uint8_t H_ZWEI[]    = {18};
static const uint8_t H_DREI[]    = {17};
static const uint8_t H_VIER[]    = {16};
static const uint8_t H_FUENF[]   = {21};
static const uint8_t H_SECHS[]   = {22, 23};
static const uint8_t H_SIEBEN[]  = {24, 25};
static const uint8_t H_ACHT[]    = {26};
static const uint8_t H_NEUN[]    = {31};
static const uint8_t H_ZEHN[]    = {30};
static const uint8_t H_ELF[]     = {29};
static const uint8_t H_ZWOELF[]  = {27, 28};
static const uint8_t FRAME_TOP[]    = {1, 2, 3};
static const uint8_t FRAME_BOTTOM[] = {37, 38, 39, 40};

// ---- Hilfsfunktionen ----------------------------------------
static void fwc_setId(uint8_t id, const CRGB& c) {
  if (id >= 1 && id <= FWC_NUM_LEDS) fwc_leds[id - 1] = c;
}
static void fwc_setIds(const uint8_t* ids, size_t n, const CRGB& c) {
  for (size_t i = 0; i < n; i++) fwc_setId(ids[i], c);
}
static void fwc_clear() { fill_solid(fwc_leds, FWC_NUM_LEDS, CRGB::Black); }
static void fwc_drawFrame() {
  fwc_setIds(FRAME_TOP,    sizeof(FRAME_TOP),    FWC_COLOR_FRAME);
  fwc_setIds(FRAME_BOTTOM, sizeof(FRAME_BOTTOM), FWC_COLOR_FRAME);
}
// Anzeigen
static void fwc_show() {
  FastLED.show();
}

// ---- Stundenwort --------------------------------------------
static uint8_t fwc_h12(uint8_t h24) { uint8_t h = h24 % 12; return h == 0 ? 12 : h; }
static void fwc_hourWord(uint8_t h12, bool useEin) {
  #define FH(arr) fwc_setIds(arr, sizeof(arr), FWC_COLOR_TIME)
  switch (h12) {
    case 1:  if (useEin) FH(H_EIN); else FH(H_EINS); break;
    case 2:  FH(H_ZWEI);   break; case 3:  FH(H_DREI);   break;
    case 4:  FH(H_VIER);   break; case 5:  FH(H_FUENF);  break;
    case 6:  FH(H_SECHS);  break; case 7:  FH(H_SIEBEN); break;
    case 8:  FH(H_ACHT);   break; case 9:  FH(H_NEUN);   break;
    case 10: FH(H_ZEHN);   break; case 11: FH(H_ELF);    break;
    case 12: FH(H_ZWOELF); break;
  }
  #undef FH
}

// ---- Zeitanzeige rendern ------------------------------------
static void fwc_render(uint8_t h24, uint8_t m) {
  fwc_clear();
  fwc_drawFrame();
  uint8_t h12  = fwc_h12(h24);
  uint8_t h12n = fwc_h12((h24 + 1) % 24);

  #define SG(w) fwc_setIds(w, sizeof(w), FWC_COLOR_TIME)

  if (m == 0) {
    if (h24 == 12) { SG(H_ZWOELF); SG(W_UHR); SG(W_MITTAGS); fwc_show(); return; }
    if (h24 == 0)  { SG(H_ZWOELF); SG(W_UHR); SG(W_NACHTS);  fwc_show(); return; }
    fwc_hourWord(h12, (h12 == 1)); SG(W_UHR);
  }
  else if (m == 1)            { SG(W_KURZ);    SG(W_NACH); fwc_hourWord(h12,  false); }
  else if (m >= 2  && m <= 4) { SG(W_GLEICH);  SG(W_FUENF_M); SG(W_NACH); fwc_hourWord(h12,  false); }
  else if (m == 5)            { SG(W_FUENF_M); SG(W_NACH); fwc_hourWord(h12,  false); }
  else if (m >= 6  && m <= 9) { SG(W_GLEICH);  SG(W_ZEHN_M);  SG(W_NACH); fwc_hourWord(h12,  false); }
  else if (m == 10)           { SG(W_ZEHN_M);  SG(W_NACH); fwc_hourWord(h12,  false); }
  else if (m >= 11 && m <= 14){ SG(W_GLEICH);  SG(W_VIERTEL); SG(W_NACH); fwc_hourWord(h12,  false); }
  else if (m == 15)           { SG(W_VIERTEL); SG(W_NACH); fwc_hourWord(h12,  false); }
  else if (m >= 16 && m <= 19){ SG(W_GLEICH);  SG(W_ZWANZIG); SG(W_NACH); fwc_hourWord(h12,  false); }
  else if (m == 20)           { SG(W_ZWANZIG); SG(W_NACH); fwc_hourWord(h12,  false); }
  else if (m >= 21 && m <= 24){ SG(W_GLEICH);  SG(W_FUENF_M); SG(W_VOR); SG(W_HALB); fwc_hourWord(h12n, false); }
  else if (m == 25)           { SG(W_FUENF_M); SG(W_VOR); SG(W_HALB); fwc_hourWord(h12n, false); }
  else if (m >= 26 && m <= 29){ SG(W_GLEICH);  SG(W_HALB); fwc_hourWord(h12n, false); }
  else if (m == 30)           { SG(W_HALB);    fwc_hourWord(h12n, false); }
  else if (m >= 31 && m <= 34){ SG(W_GLEICH);  SG(W_FUENF_M); SG(W_NACH); SG(W_HALB); fwc_hourWord(h12n, false); }
  else if (m == 35)           { SG(W_FUENF_M); SG(W_NACH); SG(W_HALB); fwc_hourWord(h12n, false); }
  else if (m >= 36 && m <= 39){ SG(W_GLEICH);  SG(W_ZWANZIG); SG(W_VOR); fwc_hourWord(h12n, false); }
  else if (m == 40)           { SG(W_ZWANZIG); SG(W_VOR); fwc_hourWord(h12n, false); }
  else if (m >= 41 && m <= 44){ SG(W_GLEICH);  SG(W_VIERTEL); SG(W_VOR); fwc_hourWord(h12n, false); }
  else if (m == 45)           { SG(W_VIERTEL); SG(W_VOR); fwc_hourWord(h12n, false); }
  else if (m >= 46 && m <= 49){ SG(W_GLEICH);  SG(W_ZEHN_M);  SG(W_VOR); fwc_hourWord(h12n, false); }
  else if (m == 50)           { SG(W_ZEHN_M);  SG(W_VOR); fwc_hourWord(h12n, false); }
  else if (m >= 51 && m <= 54){ SG(W_GLEICH);  SG(W_FUENF_M); SG(W_VOR); fwc_hourWord(h12n, false); }
  else if (m == 55)           { SG(W_FUENF_M); SG(W_VOR); fwc_hourWord(h12n, false); }
  else                        { SG(W_GLEICH);  fwc_hourWord(h12n, false); }

  #undef SG
  fwc_show();
}

// ---- Startup-Animation: Lauflicht LED 1 → 40 ---------------
static void fwc_startupAnim() {
  FastLED.setBrightness(255);
  for (uint8_t id = 1; id <= FWC_NUM_LEDS; id++) {
    fwc_clear();
    fwc_setId(id, CRGB::White);
    FastLED.show();
    delay(40);
  }
  fwc_clear();
  FastLED.show();
  FastLED.setBrightness(gBrightness);
}

// ---- Uhrzeit als Satz (für WebIF + Serial) ------------------
static String fwc_hourName(uint8_t h12, bool useEin) {
  if (h12 == 1) return useEin ? "ein" : "eins";
  const char* names[] = {"","ein","zwei","drei","vier","fuenf","sechs",
                          "sieben","acht","neun","zehn","elf","zwoelf"};
  return String(names[h12]);
}

String user_timeToWords(int h24, int m) {
  uint8_t h12  = fwc_h12((uint8_t)h24);
  uint8_t h12n = fwc_h12((uint8_t)((h24 + 1) % 24));
  String s = "";
  if (m == 0) {
    if (h24 == 12) return s + "zwoelf uhr mittags";
    if (h24 == 0)  return s + "zwoelf uhr nachts";
    return s + fwc_hourName(h12, true) + " uhr";
  }
  if (m == 1)            return s + "kurz nach "    + fwc_hourName(h12,  false);
  if (m >= 2  && m <= 4) return s + "gleich fuenf nach "     + fwc_hourName(h12,  false);
  if (m == 5)            return s + "fuenf nach "    + fwc_hourName(h12,  false);
  if (m >= 6  && m <= 9) return s + "gleich zehn nach "     + fwc_hourName(h12,  false);
  if (m == 10)           return s + "zehn nach "    + fwc_hourName(h12,  false);
  if (m >= 11 && m <= 14)return s + "gleich viertel nach "  + fwc_hourName(h12,  false);
  if (m == 15)           return s + "viertel nach " + fwc_hourName(h12,  false);
  if (m >= 16 && m <= 19)return s + "gleich zwanzig nach "  + fwc_hourName(h12,  false);
  if (m == 20)           return s + "zwanzig nach " + fwc_hourName(h12,  false);
  if (m >= 21 && m <= 24)return s + "gleich fuenf vor halb " + fwc_hourName(h12n, false);
  if (m == 25)           return s + "fuenf vor halb "+ fwc_hourName(h12n, false);
  if (m >= 26 && m <= 29)return s + "gleich halb "  + fwc_hourName(h12n, false);
  if (m == 30)           return s + "halb "          + fwc_hourName(h12n, false);
  if (m >= 31 && m <= 34)return s + "gleich fuenf nach halb "+ fwc_hourName(h12n, false);
  if (m == 35)           return s + "fuenf nach halb "+ fwc_hourName(h12n, false);
  if (m >= 36 && m <= 39)return s + "gleich zwanzig vor "   + fwc_hourName(h12n, false);
  if (m == 40)           return s + "zwanzig vor "  + fwc_hourName(h12n, false);
  if (m >= 41 && m <= 44)return s + "gleich viertel vor "   + fwc_hourName(h12n, false);
  if (m == 45)           return s + "viertel vor "  + fwc_hourName(h12n, false);
  if (m >= 46 && m <= 49)return s + "gleich zehn vor "      + fwc_hourName(h12n, false);
  if (m == 50)           return s + "zehn vor "     + fwc_hourName(h12n, false);
  if (m >= 51 && m <= 54)return s + "gleich fuenf vor "      + fwc_hourName(h12n, false);
  if (m == 55)           return s + "fuenf vor "     + fwc_hourName(h12n, false);
  return s + "gleich " + fwc_hourName(h12n, false);
}

String user_timeToWords(int h24, int m);  // forward declared above

static bool fwc_initialized = false;
static bool fwc_animDone    = false;
void user_setup() {
  FastLED.addLeds<WS2812B, FWC_DATA_PIN, GRB>(fwc_leds, FWC_NUM_LEDS);
  FastLED.setBrightness(gBrightness);
  fwc_clear(); FastLED.show();
  fwc_initialized = true;
  Serial.println("[USER] TIME2WORDS ready");
}
void user_onTimeChange(int h, int m) {
  if (!fwc_initialized) return;
  if (!fwc_animDone) { fwc_startupAnim(); fwc_animDone = true; }
  fwc_render((uint8_t)h, (uint8_t)m);
  Serial.printf("[USER] %02d:%02d – %s\n", h, m, user_timeToWords(h, m).c_str());
}
void user_onBrightnessChange(uint8_t b) {
  if (!fwc_initialized) return;
  FastLED.setBrightness(b);
  if (fwc_animDone) FastLED.show();  // nur show wenn bereits etwas im Buffer ist
}
void user_loop() {}
void user_onColorsChange() {
  if (!fwc_initialized) return;
  uint8_t h = gPreviewEnabled ? gPreviewHour   : gHour;
  uint8_t m = gPreviewEnabled ? gPreviewMinute : gMinute;
  fwc_render(h, m);
}
String user_getActiveLeds() {
  if (!fwc_initialized) return "[]";
  String s = "[";
  bool first = true;
  for (int i = 0; i < FWC_NUM_LEDS; i++) {
    if (fwc_leds[i].r || fwc_leds[i].g || fwc_leds[i].b) {
      if (!first) s += ",";
      s += (i + 1);  // 1-basiert
      first = false;
    }
  }
  return s + "]";
}
void user_apBlink(bool on) {
  if (!fwc_initialized) return;
  fwc_clear();
  if (on) {
    fwc_setIds(W_UHR, sizeof(W_UHR), CRGB(255, 140, 0));  // UHR orange
  }
  FastLED.setBrightness(255);
  FastLED.show();
}
void user_allLedsOff() {
  if (!fwc_initialized) return;
  fill_solid(fwc_leds, FWC_NUM_LEDS, CRGB::Black);
  FastLED.show();
}
void user_allLedsOn() {
  if (!fwc_initialized) return;
  fill_solid(fwc_leds, FWC_NUM_LEDS, CRGB::White);
  FastLED.setBrightness(255);
  FastLED.show();
}
bool user_ledOn(int idx) {
  if (!fwc_initialized) return false;
  fwc_clear();
  if (idx >= 1 && idx <= FWC_NUM_LEDS) fwc_leds[idx - 1] = CRGB::White;
  FastLED.setBrightness(255);
  FastLED.show();
  return true;
}

#endif // TIME_2_WORDS
