#pragma once
#include "EnvConfig.h"

#ifndef CLOCK_DEBUG_PASSWORD
#define CLOCK_DEBUG_PASSWORD ""
#endif
// ============================================================
//  config.h  –  Uhr auswählen und kompilieren, fertig.
//
//  ► DIESE ZWEI ZEILEN ANPASSEN:
// ============================================================

#ifndef ACTIVE_CLOCK
#define ACTIVE_CLOCK  BTTF3  // Uhr wählen (siehe unten)
#endif
#define FW_VERSION    "v4.7.0 by ARI"   // Firmware-Version (für alle Uhren gleich)

// ============================================================
//  Uhrennamen-Konstanten (nicht ändern)
// ============================================================
#define OBEGRANSAD         1
#define WORDCLOCK          2
#define TIME_2_WORDS       3
#define SIEBEN_SEGMENTE    4
#define CUSTOM             99
#define BTTF1             10
#define BTTF3             11
#define SPRITPREIS_CYD     12

// Kann auch per platformio.ini build_flags überschrieben werden:
//   build_flags = -D ACTIVE_CLOCK=1   (OBEGRANSAD)
//   build_flags = -D ACTIVE_CLOCK=2   (WORDCLOCK)
//   build_flags = -D ACTIVE_CLOCK=3   (TIME_2_WORDS)
//   build_flags = -D ACTIVE_CLOCK=99  (CUSTOM)

// ============================================================
//  Verfügbare Uhren:
//    OBEGRANSAD   – IKEA OBEGRÄNSAD (LED-Matrix, PWM)
//    WORDCLOCK    – ESP-WORDCLOCK (NeoPixel, WS2812B)
//    TIME_2_WORDS    – FUTURE-WORD-CLOCK (FastLED, WS2812B, 40 LEDs)
//    SIEBEN_SEGMENTE   – 7-SEGMENT-CLOCK (NeoPixel, WS2812B, 30 LEDs)
//    CUSTOM       – Eigene Uhr (Werte unten selbst setzen)
//    BTTF1        – BTTF-1 Clock (TM1637 + NeoPixel, ESP8266)
//    BTTF3        – BTTF-3 Clock (3x TM1637-Zeilen + AM/PM, ESP32)
//    SPRITPREIS_CYD – Spritpreis-Uhr mit Touchdisplay (ESP32 CYD)
// ============================================================


// ------------------------------------------------------------
//  IKEA OBEGRÄNSAD
// ------------------------------------------------------------
#if ACTIVE_CLOCK == OBEGRANSAD
  #define DEVICE_NAME         "OBEGRANSAD"
  #define WIFI_AP_NAME        "OBEGRANSAD-AP"
  #define NTP_TZ              "CET-1CEST,M3.5.0/02,M10.5.0/03"
  #define BRIGHTNESS_DEFAULT  120
  #define BRIGHTNESS_MIN        5
  #define BRIGHTNESS_MAX      250
  #define DIM_ENABLED_DEFAULT false
  #define DIM_LEVEL_DEFAULT    15
  #define DIM_START_DEFAULT   (22*60)
  #define DIM_END_DEFAULT     ( 7*60)
  // Pins
  #define P_CLA  D1
  #define P_CLK  D7
  #define P_DI   D6
  #define P_EN   D5

// ------------------------------------------------------------
//  WORDCLOCK
// ------------------------------------------------------------
#elif ACTIVE_CLOCK == WORDCLOCK
  #define DEVICE_NAME         "WORDCLOCK"
  #define WIFI_AP_NAME        "WORDCLOCK-AP"
  #define NTP_TZ              "CET-1CEST,M3.5.0/02,M10.5.0/03"
  #define BRIGHTNESS_DEFAULT  128
  #define BRIGHTNESS_MIN        5
  #define BRIGHTNESS_MAX      255
  #define DIM_ENABLED_DEFAULT false
  #define DIM_LEVEL_DEFAULT    20
  #define DIM_START_DEFAULT   (22*60)
  #define DIM_END_DEFAULT     ( 7*60)
  // Pins
  #define PIN_NEOPIXEL  D4
  #define NEO_COUNT     114

// ------------------------------------------------------------
//  TIME2WORDS
// ------------------------------------------------------------
#elif ACTIVE_CLOCK == TIME_2_WORDS
  #define DEVICE_NAME         "TIME2WORDS"
  #define WIFI_AP_NAME        "TIME2WORDS-AP"
  #define NTP_TZ              "CET-1CEST,M3.5.0/2,M10.5.0/3"
  #define BRIGHTNESS_DEFAULT  190
  #define BRIGHTNESS_MIN        5
  #define BRIGHTNESS_MAX      255
  #define DIM_ENABLED_DEFAULT true
  #define DIM_LEVEL_DEFAULT    69
  #define DIM_START_DEFAULT   (22*60)
  #define DIM_END_DEFAULT     ( 8*60)
  // Pins & LED-Anzahl
  #define FWC_DATA_PIN  D8
  #define FWC_NUM_LEDS  40

// ------------------------------------------------------------
//  SIEBEN-SEGMENTE
// ------------------------------------------------------------
#elif ACTIVE_CLOCK == SIEBEN_SEGMENTE
  #define DEVICE_NAME         "SIEBEN-SEGMENTE"
  #define WIFI_AP_NAME        "SIEBEN-SEG-AP"
  #define NTP_TZ              "CET-1CEST,M3.5.0/02,M10.5.0/03"
  #define BRIGHTNESS_DEFAULT  180
  #define BRIGHTNESS_MIN        5
  #define BRIGHTNESS_MAX      255
  #define DIM_ENABLED_DEFAULT false
  #define DIM_LEVEL_DEFAULT    20
  #define DIM_START_DEFAULT   (22*60)
  #define DIM_END_DEFAULT     ( 7*60)
  // Pins & LED-Anzahl
  #define SEG_DATA_PIN  D2
  #define SEG_NUM_LEDS_CFG 30

// ------------------------------------------------------------
//  CUSTOM
// ------------------------------------------------------------
#elif ACTIVE_CLOCK == CUSTOM
  #define DEVICE_NAME         "CUSTOM"
  #define WIFI_AP_NAME        "CUSTOM-AP"
  #define NTP_TZ              "CET-1CEST,M3.5.0/02,M10.5.0/03"
  #define BRIGHTNESS_DEFAULT  128
  #define BRIGHTNESS_MIN        5
  #define BRIGHTNESS_MAX      255
  #define DIM_ENABLED_DEFAULT false
  #define DIM_LEVEL_DEFAULT    20
  #define DIM_START_DEFAULT   (22*60)
  #define DIM_END_DEFAULT     ( 7*60)
  // Pins hier eintragen:
  // #define MY_PIN  D4

// ------------------------------------------------------------
//  BTTF1 – BTTF-1 Clock
// ------------------------------------------------------------
#elif ACTIVE_CLOCK == BTTF1
  #define DEVICE_NAME         "BTTF-1"
  #define WIFI_AP_NAME        "BTTF1-AP"
  #define NTP_TZ              "CET-1CEST,M3.5.0/02,M10.5.0/03"
  #define BRIGHTNESS_DEFAULT  200
  #define BRIGHTNESS_MIN        0
  #define BRIGHTNESS_MAX      255
  #define DIM_ENABLED_DEFAULT false
  #define DIM_LEVEL_DEFAULT    20
  #define DIM_START_DEFAULT   (22*60)
  #define DIM_END_DEFAULT     ( 7*60)
  // ── Pins ──────────────────────────────────────────────────
  #define BTTF1_NEO_PIN    D5   // NeoPixel DIN
  #define BTTF1_NEO_COUNT  48
  #define BTTF1_TM_CLK     D7   // TM1637 CLK  – alle 3 Displays gemeinsam
  #define BTTF1_TM1_DIO    D1   // Display 1: Monat/Tag  – DIO
  #define BTTF1_TM2_DIO    D2   // Display 2: Jahr       – DIO
  #define BTTF1_TM3_DIO    D3   // Display 3: Std/Min    – DIO
  // AM/PM: originale Pins 32/33 sind ESP32-only – hier auf D1 Mini anpassen
  // Nicht angeschlossen → auf -1 lassen (werden ignoriert)
  #define BTTF1_PIN_AM     -1   // AM LED (nicht angeschlossen)
  #define BTTF1_PIN_PM     -1   // PM LED (nicht angeschlossen)
  #define BTTF1_ANALOG_BTN A0   // Analoger Taster


// ------------------------------------------------------------
//  BTTF3 – BTTF-3 Clock (ESP32)
//  Zeile 1: Destination Time  (rot)
//  Zeile 2: Present Time      (grün) ← NTP-Zeit
//  Zeile 3: Last Time Departed (gelb)
// ------------------------------------------------------------
#elif ACTIVE_CLOCK == BTTF3
  #define DEVICE_NAME         "BTTF-3"
  #define WIFI_AP_NAME        "BTTF3-AP"
  #define NTP_TZ              "CET-1CEST,M3.5.0/02,M10.5.0/03"
  #define BRIGHTNESS_DEFAULT  200
  #define BRIGHTNESS_MIN        0
  #define BRIGHTNESS_MAX      255
  #define DIM_ENABLED_DEFAULT false
  #define DIM_LEVEL_DEFAULT    20
  #define DIM_START_DEFAULT   (22*60)
  #define DIM_END_DEFAULT     ( 7*60)
  // ── Zeile 1: Destination Time (rot) ──────────────────────
  #define BTTF3_R_CLK        13   // CLK alle Displays Zeile 1
  #define BTTF3_R_DIO_MD     14   // DIO Month/Day
  #define BTTF3_R_DIO_YR     15   // DIO Year
  #define BTTF3_R_DIO_HM     16   // DIO Hour/Min
  #define BTTF3_R_PIN_AM     -1   // AM LED (nicht angeschlossen)
  #define BTTF3_R_PIN_PM     -1   // PM LED (nicht angeschlossen)
  // ── Zeile 2: Present Time (grün) ─────────────────────────
  #define BTTF3_G_CLK        17
  #define BTTF3_G_DIO_MD     18
  #define BTTF3_G_DIO_YR     19
  #define BTTF3_G_DIO_HM     21
  #define BTTF3_G_PIN_AM     32   // AM LED
  #define BTTF3_G_PIN_PM     33   // PM LED
  // ── Zeile 3: Last Time Departed (gelb) ───────────────────
  #define BTTF3_Y_CLK        22
  #define BTTF3_Y_DIO_MD     23
  #define BTTF3_Y_DIO_YR     25
  #define BTTF3_Y_DIO_HM     26
  #define BTTF3_Y_PIN_AM     -1   // AM LED (nicht angeschlossen)
  #define BTTF3_Y_PIN_PM     -1   // PM LED (nicht angeschlossen)

// ------------------------------------------------------------
//  SPRITPREIS-CYD – eigenständig verwaltete Netzwerk-/Displaydienste
// ------------------------------------------------------------
#elif ACTIVE_CLOCK == SPRITPREIS_CYD
  #define DEVICE_NAME         "SPRITPREIS-UHR"
  #define WIFI_AP_NAME        "Spritpreis-Uhr"


#else
  #error "Unbekannte Uhr! ACTIVE_CLOCK in config.h prüfen."
#endif

// ============================================================
//  Gemeinsame Defines (für alle Uhren gleich)
// ============================================================
#define NTP_SERVER1       "pool.ntp.org"
#define NTP_SERVER2       "time.nist.gov"
#define NTP_SERVER3       "time.google.com"
#define NTP_RESYNC_HOUR1   3
#define NTP_RESYNC_HOUR2  15

// Settings-Datei auf LittleFS
#define SETTINGS_PATH          "/settings.json"

// Debug-Seite: Standard-Passwort (wird in LittleFS überschrieben)
#define DEBUG_PASSWORD_DEFAULT CLOCK_DEBUG_PASSWORD
