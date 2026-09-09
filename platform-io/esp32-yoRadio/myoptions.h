#ifndef myoptions_h
#define myoptions_h

#include <EnvConfig.h>

#ifndef AP_SSID
#error AP_SSID fehlt. env.example nach .env kopieren und ausfuellen.
#endif
#ifndef AP_PASSWORD
#error AP_PASSWORD fehlt. Es sind mindestens acht Zeichen erforderlich.
#endif
#define apSsid AP_SSID
#define apPassword AP_PASSWORD

// ESP32 CYD aus der Spritpreis-Uhr: ILI9341 240x320 und XPT2046.
#define L10N_LANGUAGE EN
#define DSP_MODEL DSP_ILI9341
#define DSP_HSPI true
#define TFT_CS 15
#define TFT_RST -1
#define TFT_DC 2
#define BRIGHTNESS_PIN 21

// Der Touch-Controller liegt beim CYD auf einem eigenen SPI-Bus.
#define TS_MODEL TS_MODEL_XPT2046
#define TS_CS 33
#define TS_SPIPINS 25, 39, 32, 33
#define TS_X_MIN 200
#define TS_X_MAX 3700
#define TS_Y_MIN 240
#define TS_Y_MAX 3800

// Das CYD gibt Mono-Audio ueber seinen internen DAC an GPIO 26 aus.
#define I2S_INTERNAL true
#define I2S_DOUT 26
#define I2S_DAC_CHANNEL I2S_DAC_CHANNEL_LEFT_EN
#define PLAYER_FORCE_MONO true
#define VS1053_CS 255

// Das klassische CYD hat kein PSRAM. Kleine I2S-DMA-Puffer und ein kompaktes
// Peak-Meter halten weiterhin einen zusammenhaengenden Block fuer MP3/AAC frei.
#define DMA_BUFCOUNT 4
#define DMA_BUFLEN 256
#define HIDE_WEATHER

// Kompaktes Stereo-Peak-Meter links im Player, ohne mechanische Bedienelemente.
#define CYD_LEFT_PEAK_METER

// Bei mehreren Access Points mit gleicher SSID immer den staerksten waehlen.
#define WIFI_SELECT_STRONGEST_BSSID

// Keine mechanischen Taster und keine Drehgeber: Bedienung nur per Touch.
#define BTN_LEFT 255
#define BTN_CENTER 255
#define BTN_RIGHT 255
#define BTN_UP 255
#define BTN_DOWN 255
#define BTN_MODE 255
#define ENC_BTNL 255
#define ENC_BTNB 255
#define ENC_BTNR 255
#define ENC2_BTNL 255
#define ENC2_BTNB 255
#define ENC2_BTNR 255

#define SDC_CS 255
#define LED_BUILTIN 255

#endif
