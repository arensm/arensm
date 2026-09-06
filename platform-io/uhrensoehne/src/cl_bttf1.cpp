// ============================================================
//  cl_bttf1.cpp  –  BTTF-1 Clock  (D1 Mini / ESP8266)
//
//  Pin-Belegung (in config.h):
//    BTTF1_NEO_PIN   D5  – NeoPixel DIN
//    BTTF1_TM_CLK    D7  – TM1637 CLK  (alle 3 Displays)
//    BTTF1_TM1_DIO   D1  – Display 1: Monat/Tag
//    BTTF1_TM2_DIO   D2  – Display 2: Jahr
//    BTTF1_TM3_DIO   D3  – Display 3: HH:MM
//    BTTF1_PIN_AM    -1  – AM LED (nicht angeschlossen)
//    BTTF1_PIN_PM    -1  – PM LED (nicht angeschlossen)
//    BTTF1_ANALOG_BTN A0 – Farbmodus-Taster
//
//  In config.h:  #define ACTIVE_CLOCK  BTTF1
// ============================================================
#include "config.h"
#if ACTIVE_CLOCK == BTTF1

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <TM1637Display.h>
#include "settings.h"
#include "ntp.h"

// ---- Objekte -----------------------------------------------
static Adafruit_NeoPixel pixels(BTTF1_NEO_COUNT, BTTF1_NEO_PIN, NEO_GRB + NEO_KHZ800);
static TM1637Display disp1(BTTF1_TM_CLK, BTTF1_TM1_DIO); // Monat / Tag
static TM1637Display disp2(BTTF1_TM_CLK, BTTF1_TM2_DIO); // Jahr
static TM1637Display disp3(BTTF1_TM_CLK, BTTF1_TM3_DIO); // Stunde / Minute

// ---- Zustand -----------------------------------------------
static uint8_t  colorMode    = 0;
static uint32_t lastBtnCheck = 0;

// Letzte angezeigte Werte – für Refresh nach Helligkeitsänderung
static int lastH = 0, lastM = 0;

// Farbpaletten { oben(0-5), mitte(6-11), unten(12-17) }
struct RGBColor { uint8_t r, g, b; };
static const RGBColor colorTable[4][3] = {
  { {255,  0,  0}, {160,160,  0}, {255,  0,  0} }, // 0: Rot/Gelb/Rot
  { {  0,  0,255}, {200,250,255}, {  0,  0,255} }, // 1: Blau/Eisblau/Blau
  { {255,  0, 10}, {  0, 10,255}, {255,  0, 10} }, // 2: Rosa/Blau/Rosa
  { {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0} }, // 3: Aus
};

// ---- NeoPixel mit aktuellem Modus + Helligkeit setzen ------
static void applyColorMode() {
  pixels.clear();
  for (uint8_t group = 0; group < 3; group++) {
    const RGBColor &c = colorTable[colorMode][group];
    uint8_t r = (uint16_t)c.r * gBrightness / 255;
    uint8_t g = (uint16_t)c.g * gBrightness / 255;
    uint8_t b = (uint16_t)c.b * gBrightness / 255;
    for (uint8_t i = group*6; i < group*6+6; i++) {
      pixels.setPixelColor(i, pixels.Color(r, g, b));
    }
  }
  pixels.show();
}

// ---- TM1637 Helligkeit setzen + Displays sofort neu schreiben
// TM1637 übernimmt setBrightness erst beim nächsten show-Aufruf!
static void applyTMBrightness(uint8_t b) {
  uint8_t bri7 = map(b, 0, 255, 0, 7);
  disp1.setBrightness(bri7);
  disp2.setBrightness(bri7);
  disp3.setBrightness(bri7);

  // Sofort neu schreiben damit Helligkeit aktiv wird
  int month, day, year;
  if (gPreviewEnabled) {
    month = gPreviewMonth; day = gPreviewDay; year = gPreviewYear;
  } else {
    time_t now = time(nullptr);
    struct tm *t = localtime(&now);
    month = t->tm_mon + 1; day = t->tm_mday; year = t->tm_year + 1900;
  }

  disp1.showNumberDecEx(month,  0b01000000, true, 2, 0);
  disp1.showNumberDecEx(day,    0b01000000, true, 2, 2);
  disp2.showNumberDecEx(year,   0b00000000, true);
  int _lh12 = lastH;
  if (g12hMode) { _lh12 = lastH % 12; if (_lh12 == 0) _lh12 = 12; }
  disp3.showNumberDecEx(_lh12,  0b01000000, true, 2, 0);
  disp3.showNumberDecEx(lastM,  0b01000000, true, 2, 2);
}

// ---- Displays und AM/PM aktualisieren ----------------------
static void updateDisplays(int h, int m) {
  lastH = h; lastM = m;

  // Preview-Datum nutzen falls aktiv, sonst Systemzeit
  int month, day, year;
  if (gPreviewEnabled) {
    month = gPreviewMonth;
    day   = gPreviewDay;
    year  = gPreviewYear;
  } else {
    time_t now = time(nullptr);
    struct tm *t = localtime(&now);
    month = t->tm_mon + 1;
    day   = t->tm_mday;
    year  = t->tm_year + 1900;
  }

  disp1.showNumberDecEx(month, 0b01000000, true, 2, 0);
  disp1.showNumberDecEx(day,   0b01000000, true, 2, 2);
  disp2.showNumberDecEx(year,  0b00000000, true);
  // 12h-Konvertierung für Display 3
  int h12 = h;
  if (g12hMode) { h12 = h % 12; if (h12 == 0) h12 = 12; }
  disp3.showNumberDecEx(h12,   0b01000000, true, 2, 0);
  disp3.showNumberDecEx(m,     0b01000000, true, 2, 2);

  // AM/PM: nur im 12h-Modus aktiv, sonst beide aus
  bool isPM = (h >= 12);
  if (BTTF1_PIN_AM >= 0) digitalWrite(BTTF1_PIN_AM, (g12hMode && !isPM) ? HIGH : LOW);
  if (BTTF1_PIN_PM >= 0) digitalWrite(BTTF1_PIN_PM, (g12hMode &&  isPM) ? HIGH : LOW);
}

// ============================================================
//  Hook-Implementierungen
// ============================================================

void user_setup() {
  pixels.begin();
  pixels.clear();
  pixels.show();

  if (BTTF1_PIN_AM >= 0) { pinMode(BTTF1_PIN_AM, OUTPUT); digitalWrite(BTTF1_PIN_AM, LOW); }
  if (BTTF1_PIN_PM >= 0) { pinMode(BTTF1_PIN_PM, OUTPUT); digitalWrite(BTTF1_PIN_PM, LOW); }

  uint8_t bri7 = map(gBrightness, 0, 255, 0, 7);
  disp1.setBrightness(bri7);
  disp2.setBrightness(bri7);
  disp3.setBrightness(bri7);
  disp1.clear();
  disp2.clear();
  disp3.clear();

  applyColorMode();
  Serial.println("[USER] BTTF-1 ready");
}

void user_onTimeChange(int h, int m) {
  updateDisplays(h, m);
}

void user_onBrightnessChange(uint8_t b) {
  // TM1637: Helligkeit setzen + sofort neu schreiben
  applyTMBrightness(b);
  // NeoPixel: Helligkeit über gBrightness (bereits gesetzt vom Framework)
  applyColorMode();
}

void user_loop() {
  uint32_t now = millis();
  if (now - lastBtnCheck < 50) return;
  lastBtnCheck = now;

  if (analogRead(BTTF1_ANALOG_BTN) > 100) {
    colorMode = (colorMode + 1) % 4;
    applyColorMode();
    delay(250);
  }
}

void user_apBlink(bool on) {
  if (on) {
    const uint8_t SEG_DASH = 0b01000000;
    uint8_t seg[4] = { SEG_DASH, SEG_DASH, SEG_DASH, SEG_DASH };
    disp1.setSegments(seg);
    disp2.setSegments(seg);
    disp3.setSegments(seg);
    for (uint8_t i = 0; i < 18; i++)
      pixels.setPixelColor(i, pixels.Color(255, 140, 0));
    pixels.show();
  } else {
    disp1.clear();
    disp2.clear();
    disp3.clear();
    pixels.clear();
    pixels.show();
  }
}

bool user_ledOn(int idx)   { (void)idx; return false; }
void user_allLedsOn() {
  // NeoPixel alle weiß
  for (uint8_t i = 0; i < BTTF1_NEO_COUNT; i++)
    pixels.setPixelColor(i, pixels.Color(255, 255, 255));
  pixels.show();
  // TM1637: alle Segmente an = 0xFF pro Stelle
  uint8_t all[4] = {0xFF, 0xFF, 0xFF, 0xFF};
  disp1.setSegments(all);
  disp2.setSegments(all);
  disp3.setSegments(all);
}
void user_allLedsOff() {
  pixels.clear();
  pixels.show();
  disp1.clear();
  disp2.clear();
  disp3.clear();
}
void user_onColorsChange() {}
String user_getActiveLeds(){ return "[]"; }

String user_timeToWords(int h, int m) {
  time_t now = time(nullptr);
  struct tm *t = localtime(&now);
  char buf[32];
  snprintf(buf, sizeof(buf), "%02d.%02d.%04d %02d:%02d",
    t->tm_mday, t->tm_mon+1, t->tm_year+1900, h, m);
  return String(buf);
}

#endif // BTTF1
