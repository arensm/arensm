#define LGFX_USE_V1

#include "config.h"
#if ACTIVE_CLOCK == SPRITPREIS_CYD

#include <Arduino.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <HTTPClient.h>
#include <LovyanGFX.hpp>
#include <Preferences.h>
#include <U8g2lib.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <qrcode.h>
#include <time.h>

#ifndef SPRITPREIS_WIFI_SSID
#define SPRITPREIS_WIFI_SSID ""
#endif
#ifndef SPRITPREIS_WIFI_PASSWORD
#define SPRITPREIS_WIFI_PASSWORD ""
#endif

// Heemol / CYD ESP32-2432S028R V1.2 (Dual USB)
// LCD: ST7789, 240x320, Touch: XPT2046, Audio-Verstaerker: GPIO 26.
// Einige Chargen nutzen stattdessen ILI9341. Dafuer gibt es eine zweite
// PlatformIO-Umgebung in platformio.ini.

namespace Pins {
constexpr int TFT_MISO = 12;
constexpr int TFT_MOSI = 13;
constexpr int TFT_SCLK = 14;
constexpr int TFT_CS = 15;
constexpr int TFT_DC = 2;
constexpr int TFT_BL = 21;

constexpr int TOUCH_CLK = 25;
constexpr int TOUCH_CS = 33;
constexpr int TOUCH_MOSI = 32;
constexpr int TOUCH_MISO = 39;
constexpr int TOUCH_IRQ = 36;

constexpr int SPEAKER = 26;
}  // namespace Pins

class CydDisplay : public lgfx::LGFX_Device {
 private:
  lgfx::Bus_SPI bus_;
#if defined(CYD_ILI9341)
  lgfx::Panel_ILI9341 panel_;
#else
  lgfx::Panel_ST7789 panel_;
#endif
  lgfx::Light_PWM light_;
  lgfx::Touch_XPT2046 touch_;

 public:
  CydDisplay() {
    {
      auto cfg = bus_.config();
      cfg.spi_host = HSPI_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 40000000;
      cfg.freq_read = 16000000;
      cfg.spi_3wire = true;
      cfg.use_lock = true;
      cfg.dma_channel = 1;
      cfg.pin_sclk = Pins::TFT_SCLK;
      cfg.pin_mosi = Pins::TFT_MOSI;
      cfg.pin_miso = Pins::TFT_MISO;
      cfg.pin_dc = Pins::TFT_DC;
      bus_.config(cfg);
      panel_.setBus(&bus_);
    }
    {
      auto cfg = panel_.config();
      cfg.pin_cs = Pins::TFT_CS;
      cfg.pin_rst = -1;  // TFT-Reset liegt am ESP32-EN-Signal.
      cfg.pin_busy = -1;
      cfg.memory_width = 240;
      cfg.memory_height = 320;
      cfg.panel_width = 240;
      cfg.panel_height = 320;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.offset_rotation = 0;
      cfg.readable = false;
      cfg.invert = true;
      cfg.rgb_order = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = false;
      panel_.config(cfg);
    }
    {
      auto cfg = light_.config();
      cfg.pin_bl = Pins::TFT_BL;
      cfg.invert = false;
      cfg.freq = 5000;
      cfg.pwm_channel = 7;
      light_.config(cfg);
      panel_.setLight(&light_);
    }
    {
      auto cfg = touch_.config();
      // Startwerte fuer den ueblichen XPT2046 des CYD. Unten in der Webober-
      // flaeche koennen die vier Werte ohne Neukompilieren angepasst werden.
      cfg.x_min = 200;
      cfg.x_max = 3800;
      cfg.y_min = 240;
      cfg.y_max = 3800;
      cfg.pin_int = Pins::TOUCH_IRQ;
      cfg.bus_shared = false;
      cfg.offset_rotation = 0;
      cfg.spi_host = VSPI_HOST;
      cfg.freq = 1000000;
      cfg.pin_sclk = Pins::TOUCH_CLK;
      cfg.pin_mosi = Pins::TOUCH_MOSI;
      cfg.pin_miso = Pins::TOUCH_MISO;
      cfg.pin_cs = Pins::TOUCH_CS;
      touch_.config(cfg);
      panel_.setTouch(&touch_);
    }
    setPanel(&panel_);
  }
};

CydDisplay display;
WebServer server(80);
DNSServer dnsServer;
Preferences preferences;

// Die TFT-Standardfonts enthalten nur ASCII. Diese kompakten U8g2-Schriften
// decken Latin-1 ab und stellen ä, ö, ü und ß als echtes UTF-8 dar.
const lgfx::U8g2font FONT_DE_SMALL(u8g2_font_helvR10_tf);
const lgfx::U8g2font FONT_DE_NORMAL(u8g2_font_helvR12_tf);
const lgfx::U8g2font FONT_DE_BOLD(u8g2_font_helvB14_tf);

struct AppConfig {
  String ssid;
  String password;
  String locationQuery;
  String resolvedLocation;
  double latitude = NAN;
  double longitude = NAN;
  float radiusKm = 8.0f;
  String fuel = "E10";
  String favoriteId;
  float alarmPrice = 1.70f;
  int quietStart = 22;
  int quietEnd = 7;
  float consumption = 7.0f;
  int refreshMinutes = 5;
  int touchXMin = 200;
  int touchXMax = 3800;
  int touchYMin = 240;
  int touchYMax = 3800;
};

struct Station {
  String id;
  String brand;
  String address;
  float distance = NAN;
  float price = NAN;
  float dieselPrice = NAN;
  String signal;
  String reportedAt;
  String dieselReportedAt;
  bool open = false;
};

constexpr size_t MAX_STATIONS = 24;
constexpr size_t HISTORY_SIZE = 96;
constexpr uint8_t PAGE_COUNT = 5;
AppConfig config;
Station stations[MAX_STATIONS];
size_t stationCount = 0;
float priceHistory[HISTORY_SIZE] = {};
size_t historyCount = 0;
size_t historyHead = 0;

bool accessPointMode = false;
bool fetching = false;
bool alarmLatched = false;
bool lastFetchOk = false;
String lastError;
String updatedAt;
uint32_t lastFetchMs = 0;
uint32_t lastScreenRefreshMs = 0;
uint8_t currentPage = 0;

constexpr uint16_t COLOR_BG = 0x10A2;
constexpr uint16_t COLOR_CARD = 0x2124;
constexpr uint16_t COLOR_TEXT = 0xFFFF;
constexpr uint16_t COLOR_MUTED = 0xBDF7;
constexpr uint16_t COLOR_ACCENT = 0xFD20;
constexpr uint16_t COLOR_GREEN = 0x3E67;
constexpr uint16_t COLOR_RED = 0xF986;
constexpr char API_BASE[] = "https://api.tankpuls.de";
constexpr char TZ_EUROPE_BERLIN[] = "CET-1CEST,M3.5.0,M10.5.0/3";

String htmlEscape(const String& input) {
  String out;
  out.reserve(input.length() + 16);
  for (const char c : input) {
    switch (c) {
      case '&': out += F("&amp;"); break;
      case '<': out += F("&lt;"); break;
      case '>': out += F("&gt;"); break;
      case '\"': out += F("&quot;"); break;
      case '\'': out += F("&#39;"); break;
      default: out += c;
    }
  }
  return out;
}

String urlEncode(const String& value) {
  const char* hex = "0123456789ABCDEF";
  String out;
  for (const uint8_t c : value) {
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      out += static_cast<char>(c);
    } else {
      out += '%';
      out += hex[c >> 4];
      out += hex[c & 15];
    }
  }
  return out;
}

void loadConfig() {
  // Beim allerersten Start existiert der Namespace noch nicht. Read-write legt
  // ihn einmalig an und verhindert NVS_NOT_FOUND im seriellen Monitor.
  if (!preferences.begin("fuelclock", false)) {
    Serial.println("NVS konnte nicht geoeffnet werden; Standardwerte bleiben aktiv.");
    return;
  }
  // Preferences::get* protokolliert unter Arduino-ESP32 3.x fehlende Keys als
  // Fehler. isKey() haelt den ersten Start sauber und laesst die oben gesetzten
  // Standardwerte unveraendert.
  if (preferences.isKey("ssid")) config.ssid = preferences.getString("ssid");
  if (preferences.isKey("pass")) config.password = preferences.getString("pass");
  if (preferences.isKey("place")) config.locationQuery = preferences.getString("place");
  if (preferences.isKey("resolved")) config.resolvedLocation = preferences.getString("resolved");
  if (preferences.isKey("lat")) config.latitude = preferences.getDouble("lat");
  if (preferences.isKey("lon")) config.longitude = preferences.getDouble("lon");
  if (preferences.isKey("radius")) config.radiusKm = preferences.getFloat("radius");
  if (preferences.isKey("fuel")) config.fuel = preferences.getString("fuel");
  if (preferences.isKey("favorite")) config.favoriteId = preferences.getString("favorite");
  if (preferences.isKey("alarm")) config.alarmPrice = preferences.getFloat("alarm");
  if (preferences.isKey("quiet_s")) config.quietStart = preferences.getInt("quiet_s");
  if (preferences.isKey("quiet_e")) config.quietEnd = preferences.getInt("quiet_e");
  if (preferences.isKey("consume")) config.consumption = preferences.getFloat("consume");
  if (preferences.isKey("refresh")) config.refreshMinutes = preferences.getInt("refresh");
  if (preferences.isKey("txmin")) config.touchXMin = preferences.getInt("txmin");
  if (preferences.isKey("txmax")) config.touchXMax = preferences.getInt("txmax");
  if (preferences.isKey("tymin")) config.touchYMin = preferences.getInt("tymin");
  if (preferences.isKey("tymax")) config.touchYMax = preferences.getInt("tymax");
  preferences.end();

  // Optionale Erstkonfiguration aus der nicht versionierten .env. Sobald im
  // Captive Portal WLAN-Daten gespeichert wurden, hat NVS Vorrang.
  if (config.ssid.isEmpty() && SPRITPREIS_WIFI_SSID[0] != '\0') {
    config.ssid = SPRITPREIS_WIFI_SSID;
    config.password = SPRITPREIS_WIFI_PASSWORD;
  }
}

void saveConfig() {
  preferences.begin("fuelclock", false);
  preferences.putString("ssid", config.ssid);
  preferences.putString("pass", config.password);
  preferences.putString("place", config.locationQuery);
  preferences.putString("resolved", config.resolvedLocation);
  preferences.putDouble("lat", config.latitude);
  preferences.putDouble("lon", config.longitude);
  preferences.putFloat("radius", config.radiusKm);
  preferences.putString("fuel", config.fuel);
  preferences.putString("favorite", config.favoriteId);
  preferences.putFloat("alarm", config.alarmPrice);
  preferences.putInt("quiet_s", config.quietStart);
  preferences.putInt("quiet_e", config.quietEnd);
  preferences.putFloat("consume", config.consumption);
  preferences.putInt("refresh", config.refreshMinutes);
  preferences.putInt("txmin", config.touchXMin);
  preferences.putInt("txmax", config.touchXMax);
  preferences.putInt("tymin", config.touchYMin);
  preferences.putInt("tymax", config.touchYMax);
  preferences.end();
}

String localAddress() {
  return accessPointMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
}

String deviceUrl() { return "http://" + localAddress() + "/"; }

void beep(uint16_t frequency = 2200, uint16_t duration = 180) {
  tone(Pins::SPEAKER, frequency, duration);
  delay(duration + 20);
  noTone(Pins::SPEAKER);
}

bool isQuietTime() {
  struct tm now {};
  if (!getLocalTime(&now, 20)) return false;
  const int hour = now.tm_hour;
  if (config.quietStart == config.quietEnd) return false;
  if (config.quietStart < config.quietEnd) {
    return hour >= config.quietStart && hour < config.quietEnd;
  }
  return hour >= config.quietStart || hour < config.quietEnd;
}

String fuelJsonKey() {
  String key = config.fuel;
  key.toLowerCase();
  return key;
}

String tankPulsUrl(const String& fuel) {
  const double latitudeDelta = config.radiusKm / 111.0;
  const double longitudeScale = 111.0 * cos(config.latitude * DEG_TO_RAD);
  const double longitudeDelta = config.radiusKm / longitudeScale;
  return String(API_BASE) + "/api/search/cheapest?minLat=" +
         String(config.latitude - latitudeDelta, 6) +
         "&minLng=" + String(config.longitude - longitudeDelta, 6) +
         "&maxLat=" + String(config.latitude + latitudeDelta, 6) +
         "&maxLng=" + String(config.longitude + longitudeDelta, 6) +
         "&fuel=" + fuel + "&limit=50";
}

bool fetchDieselPrices() {
  for (size_t i = 0; i < stationCount; ++i) {
    stations[i].dieselPrice = NAN;
    stations[i].dieselReportedAt = "";
  }

  if (config.fuel.equalsIgnoreCase("Diesel")) {
    for (size_t i = 0; i < stationCount; ++i) {
      stations[i].dieselPrice = stations[i].price;
      stations[i].dieselReportedAt = stations[i].reportedAt;
    }
    return stationCount > 0;
  }

  WiFiClientSecure tls;
  tls.setInsecure();
  HTTPClient https;
  https.setConnectTimeout(8000);
  https.setTimeout(12000);
  if (!https.begin(tls, tankPulsUrl("diesel"))) return false;

  https.addHeader("Accept", "application/json");
  https.addHeader("User-Agent", "Spritpreis-Uhr-CYD/1.3");
  const int status = https.GET();
  bool matched = false;
  if (status == HTTP_CODE_OK) {
    const String payload = https.getString();
    Serial.printf("TankPuls Diesel HTTP 200, %u Bytes\n",
                  static_cast<unsigned>(payload.length()));
    if (!payload.isEmpty()) {
      JsonDocument doc;
      if (!deserializeJson(doc, payload)) {
        JsonArray list = doc["items"].as<JsonArray>();
        for (JsonObject item : list) {
          const String id = item["id"] | "";
          if (id.isEmpty()) continue;
          for (size_t i = 0; i < stationCount; ++i) {
            if (stations[i].id != id) continue;
            stations[i].dieselPrice = !item["priceCents"].isNull()
                                          ? item["priceCents"].as<float>() / 1000.0f
                                          : NAN;
            stations[i].dieselReportedAt = item["priceTs"] | "";
            matched = matched || !isnan(stations[i].dieselPrice);
            break;
          }
        }
      }
    }
  } else {
    Serial.printf("TankPuls Diesel HTTP %d\n", status);
  }
  https.end();
  return matched;
}

int cheapestIndex() {
  int found = -1;
  for (size_t i = 0; i < stationCount; ++i) {
    if (isnan(stations[i].price)) continue;
    if (found < 0 || stations[i].price < stations[found].price) found = i;
  }
  return found;
}

int nearestIndex() {
  int found = -1;
  for (size_t i = 0; i < stationCount; ++i) {
    if (isnan(stations[i].distance)) continue;
    if (found < 0 || stations[i].distance < stations[found].distance) found = i;
  }
  return found;
}

int favoriteIndex() {
  if (config.favoriteId.isEmpty()) return cheapestIndex();
  for (size_t i = 0; i < stationCount; ++i) {
    if (stations[i].id == config.favoriteId) return static_cast<int>(i);
  }
  return cheapestIndex();
}

void recordHistory(float price) {
  if (isnan(price)) return;
  priceHistory[historyHead] = price;
  historyHead = (historyHead + 1) % HISTORY_SIZE;
  if (historyCount < HISTORY_SIZE) ++historyCount;
}

void checkAlarm() {
  const int index = cheapestIndex();
  if (index < 0 || config.alarmPrice <= 0) return;
  const float price = stations[index].price;
  if (price > config.alarmPrice + 0.01f) alarmLatched = false;
  if (price <= config.alarmPrice && !alarmLatched && !isQuietTime()) {
    beep(2200, 180);
    delay(80);
    beep(2800, 260);
    alarmLatched = true;
  }
}

bool geocodeLocation(const String& query, double& latitude, double& longitude,
                     String& resolved, String& errorText) {
  if (WiFi.status() != WL_CONNECTED) {
    errorText = "Keine WLAN-Verbindung";
    return false;
  }

  String search = query;
  search.trim();
  if (search.isEmpty()) {
    errorText = "Bitte eine PLZ oder einen Ort eingeben";
    return false;
  }

  const String url = "https://nominatim.openstreetmap.org/search?q=" +
                     urlEncode(search + ", Deutschland") +
                     "&format=jsonv2&limit=1&countrycodes=de&addressdetails=0";
  WiFiClientSecure tls;
  tls.setInsecure();
  HTTPClient https;
  https.setConnectTimeout(8000);
  https.setTimeout(12000);
  if (!https.begin(tls, url)) {
    errorText = "Ortssuche konnte nicht gestartet werden";
    return false;
  }

  https.addHeader("Accept", "application/json");
  https.addHeader("Accept-Language", "de");
  https.addHeader("User-Agent", "Spritpreis-Uhr-CYD/1.2 (local ESP32 device)");
  const int status = https.GET();
  bool ok = false;
  if (status == HTTP_CODE_OK) {
    JsonDocument doc;
    const DeserializationError jsonError = deserializeJson(doc, https.getStream());
    JsonArray results = doc.as<JsonArray>();
    if (jsonError) {
      errorText = "Ortssuche: ungueltige Antwort";
    } else if (results.isNull() || results.size() == 0) {
      errorText = "Kein passender Ort in Deutschland gefunden";
    } else {
      const char* latText = results[0]["lat"] | "";
      const char* lonText = results[0]["lon"] | "";
      latitude = String(latText).toDouble();
      longitude = String(lonText).toDouble();
      resolved = results[0]["display_name"] | search;
      ok = latitude >= -90.0 && latitude <= 90.0 &&
           longitude >= -180.0 && longitude <= 180.0;
      if (!ok) errorText = "Ortssuche lieferte ungueltige Koordinaten";
    }
  } else {
    errorText = "Ortssuche HTTP " + String(status);
  }
  https.end();
  return ok;
}

bool fetchStations() {
  if (WiFi.status() != WL_CONNECTED || fetching) return false;
  if (config.locationQuery.isEmpty() || isnan(config.latitude) ||
      isnan(config.longitude)) {
    lastError = "Bitte zuerst Ort oder PLZ konfigurieren";
    lastFetchOk = false;
    // Verhindert, dass loop() ohne Standort fortlaufend neu zeichnet.
    lastFetchMs = millis();
    return false;
  }
  fetching = true;
  lastError = "";

  // Die aktuell von tankpuls.de selbst verwendete API erwartet eine Bounding-
  // Box statt Mittelpunkt und Radius. Nach dem Abruf filtern wir zusätzlich
  // kreisförmig auf den eingestellten Radius.
  const String url = tankPulsUrl(fuelJsonKey());

  WiFiClientSecure tls;
  // TankPuls liefert keine Zugangsdaten. setInsecure vermeidet, dass ein spaeter
  // ablaufendes Root-Zertifikat die Uhr stilllegt; Preise sind damit allerdings
  // nicht kryptografisch gegen Manipulation im lokalen Netz abgesichert.
  tls.setInsecure();
  HTTPClient https;
  https.setConnectTimeout(8000);
  https.setTimeout(12000);

  bool ok = false;
  if (!https.begin(tls, url)) {
    lastError = "HTTPS konnte nicht gestartet werden";
  } else {
    https.addHeader("Accept", "application/json");
    https.addHeader("User-Agent", "Spritpreis-Uhr-CYD/1.2");
    const int status = https.GET();
    if (status == HTTP_CODE_OK) {
      // Bei längeren Antworten lieferte das direkte Parsen von getStream()
      // auf einzelnen ESP32/Arduino-Core-Kombinationen gelegentlich EmptyInput,
      // obwohl HTTP 200 und Content-Length vorhanden waren. Erst vollständig
      // einlesen, dann parsen; so lässt sich eine echte Leerantwort unterscheiden.
      const String payload = https.getString();
      Serial.printf("TankPuls HTTP 200, %u Bytes\n",
                    static_cast<unsigned>(payload.length()));
      if (payload.isEmpty()) {
        lastError = "TankPuls: leere Antwort (HTTP 200)";
      } else {
        JsonDocument doc;
        const DeserializationError error = deserializeJson(doc, payload);
        if (error) {
          lastError = "JSON: " + String(error.c_str());
        } else {
          JsonArray list = doc["items"].as<JsonArray>();
          if (list.isNull()) {
            lastError = "API-Antwort enthaelt keine items-Liste";
          } else {
            stationCount = 0;
            for (JsonObject item : list) {
              if (stationCount >= MAX_STATIONS) break;
              const double stationLat = item["lat"] | NAN;
              const double stationLng = item["lng"] | NAN;
              if (isnan(stationLat) || isnan(stationLng)) continue;

              const double latRadians1 = config.latitude * DEG_TO_RAD;
              const double latRadians2 = stationLat * DEG_TO_RAD;
              const double deltaLat = (stationLat - config.latitude) * DEG_TO_RAD;
              const double deltaLon = (stationLng - config.longitude) * DEG_TO_RAD;
              const double haversineA = sin(deltaLat / 2.0) * sin(deltaLat / 2.0) +
                                        cos(latRadians1) * cos(latRadians2) *
                                        sin(deltaLon / 2.0) * sin(deltaLon / 2.0);
              const float distanceKm =
                  6371.0 * 2.0 * asin(sqrt(haversineA));
              if (distanceKm > config.radiusKm) continue;

              Station& station = stations[stationCount++];
              station.id = item["id"] | "";
              station.brand = item["brandName"] | "";
              if (station.brand.isEmpty()) station.brand = item["name"] | "Unbekannt";
              station.address = String(item["street"] | "") + ", " +
                                String(item["postcode"] | "") + " " +
                                String(item["city"] | "");
              station.distance = distanceKm;
              station.price = !item["priceCents"].isNull()
                                  ? item["priceCents"].as<float>() / 1000.0f
                                  : NAN;
              station.dieselPrice = NAN;
              station.signal = "avg";
              station.reportedAt = item["priceTs"] | "";
              station.dieselReportedAt = "";
              station.open = String(item["status"] | "") == "open";
            }

            // Farbliche Preisbewertung relativ zum Median der gefundenen Preise.
            float sortedPrices[MAX_STATIONS];
            size_t priceCount = 0;
            for (size_t i = 0; i < stationCount; ++i) {
              if (!isnan(stations[i].price)) sortedPrices[priceCount++] = stations[i].price;
            }
            for (size_t i = 1; i < priceCount; ++i) {
              const float value = sortedPrices[i];
              size_t j = i;
              while (j > 0 && sortedPrices[j - 1] > value) {
                sortedPrices[j] = sortedPrices[j - 1];
                --j;
              }
              sortedPrices[j] = value;
            }
            if (priceCount > 0) {
              const float median = sortedPrices[priceCount / 2];
              for (size_t i = 0; i < stationCount; ++i) {
                if (isnan(stations[i].price)) continue;
                const int differenceCents =
                    static_cast<int>(round((stations[i].price - median) * 100.0f));
                if (differenceCents <= -3) stations[i].signal = "low";
                else if (differenceCents >= 6) stations[i].signal = "xhigh";
                else if (differenceCents >= 2) stations[i].signal = "high";
              }
            }
            updatedAt = stationCount > 0 ? stations[0].reportedAt : "";
            ok = stationCount > 0;
            if (!ok) lastError = "Keine Tankstellen im Suchradius";
          }
        }
      }
    } else {
      lastError = "TankPuls HTTP " + String(status);
    }
    https.end();
  }

  if (ok && !fetchDieselPrices()) {
    Serial.println("Kein Dieselpreis fuer gefundene Tankstellen");
  }

  lastFetchOk = ok;
  lastFetchMs = millis();
  if (ok) {
    const int index = cheapestIndex();
    if (index >= 0) recordHistory(stations[index].price);
    checkAlarm();
  }
  fetching = false;
  return ok;
}

uint16_t signalColor(const Station& station) {
  if (!station.open) return COLOR_MUTED;
  if (station.signal == "low") return COLOR_GREEN;
  if (station.signal == "high" || station.signal == "xhigh") return COLOR_RED;
  return COLOR_ACCENT;
}

String shortened(const String& value, size_t maxLength) {
  if (value.length() <= maxLength) return value;
  size_t end = maxLength - 2;
  // UTF-8 niemals mitten in einer Mehrbyte-Sequenz abschneiden.
  while (end > 0 && (static_cast<uint8_t>(value[end]) & 0xC0) == 0x80) --end;
  return value.substring(0, end) + "..";
}

void drawFooter() {
  display.setTextDatum(lgfx::bottom_center);
  display.setTextColor(COLOR_MUTED, COLOR_BG);
  display.setFont(&FONT_DE_SMALL);
  display.drawString("TankPuls · MTS-K", 120, 319);
}

void drawHeader(const String& title, uint8_t page) {
  display.fillScreen(COLOR_BG);
  display.fillRoundRect(7, 6, 226, 34, 7, COLOR_CARD);
  display.setTextDatum(lgfx::middle_left);
  display.setTextColor(COLOR_TEXT, COLOR_CARD);
  display.setFont(&FONT_DE_BOLD);
  display.drawString(title, 14, 23);
  display.setTextDatum(lgfx::middle_right);
  display.setTextColor(WiFi.status() == WL_CONNECTED ? COLOR_GREEN : COLOR_RED,
                       COLOR_CARD);
  display.setFont(&FONT_DE_SMALL);
  display.drawString(String(page + 1) + "/" + String(PAGE_COUNT), 225, 23);
}

void drawNoData(const String& title) {
  drawHeader(title, currentPage);
  display.setTextDatum(lgfx::middle_center);
  display.setTextColor(COLOR_TEXT, COLOR_BG);
  display.setFont(&FONT_DE_NORMAL);
  display.drawString(fetching ? "Tankpreise werden geladen ..."
                              : "Noch keine Tankpreise",
                     120, 108);
  display.setTextColor(COLOR_MUTED, COLOR_BG);
  display.setFont(&FONT_DE_SMALL);
  display.drawString(lastError.isEmpty() ? "Konfiguration im Browser öffnen"
                                         : shortened(lastError, 31),
                     120, 145);
  display.setTextColor(COLOR_TEXT, COLOR_BG);
  display.drawString(deviceUrl(), 120, 176);
  display.drawString("Links / rechts tippen", 120, 245);
  drawFooter();
}

void drawStationPage(const String& title, int index, bool showDiesel = false) {
  if (index < 0 || static_cast<size_t>(index) >= stationCount) {
    drawNoData(title);
    return;
  }
  const Station& station = stations[index];
  const float shownPrice = showDiesel ? station.dieselPrice : station.price;
  const String fuelLabel = showDiesel ? "Diesel" : config.fuel;
  drawHeader(title, currentPage);

  display.setTextDatum(lgfx::top_left);
  display.setTextColor(COLOR_TEXT, COLOR_BG);
  display.setFont(&FONT_DE_BOLD);
  display.drawString(shortened(station.brand, 25), 10, 50);

  display.setFont(&FONT_DE_SMALL);
  display.setTextColor(COLOR_MUTED, COLOR_BG);
  display.drawString(shortened(station.address, 31), 10, 79);

  display.fillRoundRect(8, 106, 224, 100, 10, COLOR_CARD);
  display.setTextDatum(lgfx::middle_center);
  display.setTextColor(showDiesel
                           ? (station.open ? COLOR_ACCENT : COLOR_MUTED)
                           : signalColor(station),
                       COLOR_CARD);
  display.setFont(&fonts::Font7);
  const String priceText = isnan(shownPrice) ? "--.--" : String(shownPrice, 3);
  display.drawString(priceText, 120, 145);
  display.setFont(&FONT_DE_NORMAL);
  display.drawString("EUR/L  ·  " + fuelLabel, 120, 190);

  display.fillRoundRect(8, 214, 224, 62, 10, COLOR_CARD);
  display.setTextDatum(lgfx::middle_center);
  display.setTextColor(COLOR_MUTED, COLOR_CARD);
  display.setFont(&FONT_DE_SMALL);
  display.drawString(station.open ? "GEÖFFNET" : "GESCHLOSSEN", 45, 231);
  display.setTextColor(COLOR_TEXT, COLOR_CARD);
  display.setFont(&FONT_DE_NORMAL);
  display.drawString(isnan(station.distance) ? "-- km"
                                            : String(station.distance, 1) + " km",
                     120, 231);
  display.setFont(&FONT_DE_SMALL);
  const float travelCost = station.distance * 2.0f * config.consumption / 100.0f *
                           shownPrice;
  display.drawString(isnan(travelCost) ? "Fahrt --"
                                      : String(travelCost, 2) + " EUR Fahrt",
                     120, 260);

  display.setTextDatum(lgfx::middle_center);
  display.setFont(&FONT_DE_SMALL);
  display.setTextColor(COLOR_MUTED, COLOR_BG);
  display.drawString("< zurück              weiter >", 120, 296);
  drawFooter();
}

void drawQrCode(const String& text, int left, int top, int scale) {
  QRCode qr;
  uint8_t data[qrcode_getBufferSize(3)];
  qrcode_initText(&qr, data, 3, ECC_LOW, text.c_str());
  display.fillRect(left - 3, top - 3, qr.size * scale + 6, qr.size * scale + 6,
                   TFT_WHITE);
  for (uint8_t y = 0; y < qr.size; ++y) {
    for (uint8_t x = 0; x < qr.size; ++x) {
      if (qrcode_getModule(&qr, x, y)) {
        display.fillRect(left + x * scale, top + y * scale, scale, scale, TFT_BLACK);
      }
    }
  }
}

void drawHistoryPage() {
  drawHeader("VERLAUF & SETUP", currentPage);
  display.fillRoundRect(8, 47, 224, 126, 9, COLOR_CARD);
  display.setTextColor(COLOR_MUTED, COLOR_CARD);
  display.setFont(&FONT_DE_SMALL);
  display.setTextDatum(lgfx::top_left);
  display.drawString("Günstigster Preis seit Start", 15, 53);

  if (historyCount >= 2) {
    float minPrice = 99.0f;
    float maxPrice = 0.0f;
    for (size_t n = 0; n < historyCount; ++n) {
      const size_t i = (historyHead + HISTORY_SIZE - historyCount + n) % HISTORY_SIZE;
      minPrice = min(minPrice, priceHistory[i]);
      maxPrice = max(maxPrice, priceHistory[i]);
    }
    if (maxPrice - minPrice < 0.01f) maxPrice = minPrice + 0.01f;
    int previousX = 16;
    int previousY = 159;
    for (size_t n = 0; n < historyCount; ++n) {
      const size_t i = (historyHead + HISTORY_SIZE - historyCount + n) % HISTORY_SIZE;
      const int x = 16 + (208 * n) / max<size_t>(1, historyCount - 1);
      const int y = 159 - static_cast<int>(79.0f *
                    (priceHistory[i] - minPrice) / (maxPrice - minPrice));
      if (n) display.drawLine(previousX, previousY, x, y, COLOR_ACCENT);
      previousX = x;
      previousY = y;
    }
    display.setTextColor(COLOR_TEXT, COLOR_CARD);
    display.drawString(String(maxPrice, 3), 15, 70);
    display.drawString(String(minPrice, 3), 15, 148);
  } else {
    display.setTextDatum(lgfx::middle_center);
    display.setTextColor(COLOR_MUTED, COLOR_CARD);
    display.drawString("Verlauf nach weiteren Abrufen", 120, 111);
  }

  const String url = deviceUrl();
  drawQrCode(url, 16, 190, 3);
  display.setTextDatum(lgfx::middle_center);
  display.setTextColor(COLOR_TEXT, COLOR_BG);
  display.setFont(&FONT_DE_NORMAL);
  display.drawString(localAddress(), 169, 211);
  display.setTextColor(COLOR_MUTED, COLOR_BG);
  display.setFont(&FONT_DE_SMALL);
  display.drawString("Konfiguration", 169, 238);
  display.drawString("QR-Code scannen", 169, 258);
  display.drawString("< zurück       weiter >", 120, 296);
  drawFooter();
}

void drawWifiSetupPage() {
  display.fillScreen(COLOR_BG);
  display.fillRoundRect(7, 6, 226, 34, 7, COLOR_CARD);
  display.setTextDatum(lgfx::middle_center);
  display.setTextColor(COLOR_ACCENT, COLOR_CARD);
  display.setFont(&FONT_DE_BOLD);
  display.drawString("WLAN EINRICHTEN", 120, 23);

  display.setTextColor(COLOR_TEXT, COLOR_BG);
  display.setFont(&FONT_DE_NORMAL);
  display.drawString("Verbinden Sie sich", 120, 78);
  display.drawString("mit dem AP:", 120, 104);
  display.setTextColor(COLOR_ACCENT, COLOR_BG);
  display.setFont(&FONT_DE_BOLD);
  display.drawString("Spritpreis-Uhr", 120, 142);

  display.setTextColor(COLOR_MUTED, COLOR_BG);
  display.setFont(&FONT_DE_SMALL);
  display.drawString("Die Anmeldeseite öffnet", 120, 195);
  display.drawString("sich automatisch.", 120, 216);
  display.drawString("Sonst im Browser öffnen:", 120, 254);
  display.setTextColor(COLOR_TEXT, COLOR_BG);
  display.drawString("http://192.168.4.1", 120, 278);
  drawFooter();
}

void drawCurrentPage() {
  if (accessPointMode) {
    drawWifiSetupPage();
    return;
  }
  switch (currentPage) {
    case 0: drawStationPage(config.favoriteId.isEmpty() ? "AUSGEWÄHLT"
                                                        : "AUSGEWÄHLT",
                            favoriteIndex()); break;
    case 1: drawStationPage("DIESEL", favoriteIndex(), true); break;
    case 2: drawStationPage("GÜNSTIGSTE", cheapestIndex()); break;
    case 3: drawStationPage("NÄCHSTE", nearestIndex()); break;
    default: drawHistoryPage(); break;
  }
}

String fuelSelected(const char* value) {
  return config.fuel.equalsIgnoreCase(value) ? " selected" : "";
}

void handleFuelConfig() {
  String html;
  html.reserve(9000);
  html += F(R"HTML(<!doctype html><html lang="de"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Spritpreis-Uhr</title><style>
:root{color-scheme:dark}body{font:16px system-ui;margin:0;background:#111827;color:#f9fafb}
main{max-width:720px;margin:auto;padding:20px}.card{background:#1f2937;border-radius:14px;padding:18px;margin:14px 0}
h1{color:#f59e0b}label{display:block;margin:12px 0 5px}input,select,button{box-sizing:border-box;width:100%;padding:11px;border-radius:8px;border:1px solid #4b5563;background:#111827;color:#fff}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:12px}button{background:#d97706;border:0;font-weight:700;margin-top:16px;cursor:pointer}.small{color:#9ca3af;font-size:.9rem}.ok{color:#4ade80}.bad{color:#fb7185}a{color:#fbbf24}@media(max-width:550px){.grid{grid-template-columns:1fr}}</style></head><body><main>
<h1>Spritpreis-Uhr</h1><div class="card"><b>Status:</b> )HTML");
  html += WiFi.status() == WL_CONNECTED ? F("<span class=\"ok\">WLAN verbunden</span>")
                                        : F("<span class=\"bad\">Einrichtungsmodus</span>");
  html += "<br>Adresse: <a href=\"" + deviceUrl() + "\">" + deviceUrl() + "</a>";
  html += "<br>Tankpreise: " + String(lastFetchOk ? "OK" : "noch nicht verfügbar");
  if (!lastError.isEmpty()) html += "<br>Hinweis: " + htmlEscape(lastError);
  html += F(R"HTML(</div><form class="card" method="post" action="/save"><h2>Standort und Suche</h2>
<label>PLZ oder Ort</label><input name="place" required placeholder="z. B. 53359 Rheinbach oder Bonn" value=")HTML");
  html += htmlEscape(config.locationQuery) + F("\">");
  if (!config.resolvedLocation.isEmpty()) {
    html += "<p class=\"small\">Gefunden: " + htmlEscape(config.resolvedLocation) + "</p>";
  }
  html += F(R"HTML(<div class="grid"><div><label>Radius (1–25 km)</label><input name="radius" type="number" min="1" max="25" step="0.5" value=")HTML");
  html += String(config.radiusKm, 1) + F("\"></div><div><label>Kraftstoff</label><select name=\"fuel\"><option");
  html += fuelSelected("E5") + F(">E5</option><option");
  html += fuelSelected("E10") + F(">E10</option><option");
  html += fuelSelected("Diesel") + F(">Diesel</option></select></div></div>");
  html += F(R"HTML(<label>Bevorzugte Tankstelle (leer = günstigste)</label>
<input name="favorite" list="stations" value=")HTML");
  html += htmlEscape(config.favoriteId) + F("\"><datalist id=\"stations\">");
  for (size_t i = 0; i < stationCount; ++i) {
    html += "<option value=\"" + htmlEscape(stations[i].id) + "\">" +
            htmlEscape(stations[i].brand) + " · " + String(stations[i].distance, 1) +
            " km</option>";
  }
  html += F(R"HTML(</datalist><p class="small">Nach dem ersten erfolgreichen Abruf stehen hier die gefundenen Tankstellen zur Auswahl.</p>
<h2>Alarm und Berechnung</h2><div class="grid"><div><label>Alarm ab Preis (€/L; 0 = aus)</label><input name="alarm" type="number" min="0" max="9" step="0.001" value=")HTML");
  html += String(config.alarmPrice, 3) + F("\"></div><div><label>Verbrauch (L/100 km)</label><input name=\"consume\" type=\"number\" min=\"1\" max=\"30\" step=\"0.1\" value=\"");
  html += String(config.consumption, 1) + F("\"></div><div><label>Ruhezeit ab (Stunde)</label><input name=\"quiet_s\" type=\"number\" min=\"0\" max=\"23\" value=\"");
  html += String(config.quietStart) + F("\"></div><div><label>Ruhezeit bis (Stunde)</label><input name=\"quiet_e\" type=\"number\" min=\"0\" max=\"23\" value=\"");
  html += String(config.quietEnd) + F("\"></div><div><label>Aktualisierung (Minuten)</label><input name=\"refresh\" type=\"number\" min=\"2\" max=\"60\" value=\"");
  html += String(config.refreshMinutes) + F("\"></div></div>");
  html += F(R"HTML(<details><summary>Touch-Kalibrierung</summary><div class="grid">
<div><label>X min</label><input name="txmin" type="number" value=")HTML");
  html += String(config.touchXMin) + F("\"></div><div><label>X max</label><input name=\"txmax\" type=\"number\" value=\"");
  html += String(config.touchXMax) + F("\"></div><div><label>Y min</label><input name=\"tymin\" type=\"number\" value=\"");
  html += String(config.touchYMin) + F("\"></div><div><label>Y max</label><input name=\"tymax\" type=\"number\" value=\"");
  html += String(config.touchYMax) + F("\"></div></div></details>");
  html += F(R"HTML(<button type="submit">Konfiguration speichern</button></form>
<div class="grid"><form method="post" action="/refresh"><button>Preise jetzt abrufen</button></form>
<form method="post" action="/beep"><button>Testton</button></form></div>
<form method="post" action="/wifi-reset" class="card"><h2>WLAN wechseln</h2>
<p class="small">Löscht nur die WLAN-Zugangsdaten und startet den Einrichtungs-AP erneut.</p>
<button type="submit">WLAN neu einrichten</button></form>
<p class="small">Ortssuche: OpenStreetMap Nominatim. Preisdaten: TankPuls · MTS-K.</p>
</main></body></html>)HTML");
  server.send(200, "text/html; charset=utf-8", html);
}

void handleWifiSetup() {
  String html;
  html.reserve(7000);
  html += F(R"HTML(<!doctype html><html lang="de"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>WLAN einrichten</title><style>
:root{color-scheme:dark}body{font:16px system-ui;margin:0;background:#111827;color:#f9fafb}
main{max-width:560px;margin:auto;padding:20px}.card{background:#1f2937;border-radius:14px;padding:18px;margin:14px 0}
h1{color:#f59e0b}label{display:block;margin:12px 0 5px}select,input,button{box-sizing:border-box;width:100%;padding:12px;border-radius:8px;border:1px solid #4b5563;background:#111827;color:#fff}
button{background:#d97706;border:0;font-weight:700;margin-top:18px}.small{color:#9ca3af;font-size:.9rem}</style></head><body><main>
<h1>Spritpreis-Uhr</h1><div class="card"><h2>Mit dem eigenen WLAN verbinden</h2>
<p>WLAN auswählen und das zugehörige Passwort eingeben.</p><form method="post" action="/wifi-save">
<label>Gefundene WLANs</label><select name="ssid" required><option value="">Bitte auswählen …</option>)HTML");

  int networkCount = WiFi.scanComplete();
  bool scanPending = networkCount == WIFI_SCAN_RUNNING;
  if (networkCount == WIFI_SCAN_FAILED) {
    WiFi.scanNetworks(true, true);
    scanPending = true;
  }
  if (networkCount > 0) {
    for (int i = 0; i < networkCount; ++i) {
      const String ssid = WiFi.SSID(i);
      if (ssid.isEmpty()) continue;
      bool duplicate = false;
      for (int earlier = 0; earlier < i; ++earlier) {
        if (WiFi.SSID(earlier) == ssid) {
          duplicate = true;
          break;
        }
      }
      if (duplicate) continue;
      html += "<option value=\"" + htmlEscape(ssid) + "\">" + htmlEscape(ssid) +
              " (" + String(WiFi.RSSI(i)) + " dBm";
      html += WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? ", offen)</option>"
                                                       : ", geschützt)</option>";
    }
  } else if (scanPending) {
    html += F("<option value=\"\" disabled>WLAN-Suche läuft …</option>");
  } else {
    html += F("<option value=\"\" disabled>Keine WLANs gefunden</option>");
  }
  html += F(R"HTML(</select><label>WLAN-Passwort</label>
<input name="password" type="password" autocomplete="current-password" placeholder="Bei offenem WLAN leer lassen">
<button type="submit">Verbinden</button></form>
<p class="small">Falls Ihr WLAN fehlt, die Seite neu laden. Es werden nur die WLAN-Zugangsdaten gespeichert.</p>
</div>)HTML");
  if (scanPending) {
    html += F("<script>setTimeout(()=>location.reload(),2500)</script>");
  }
  html += F("</main></body></html>");
  server.send(200, "text/html; charset=utf-8", html);
}

void handleRoot() {
  if (accessPointMode) {
    handleWifiSetup();
  } else {
    handleFuelConfig();
  }
}

float boundedFloat(const String& name, float fallback, float low, float high) {
  if (!server.hasArg(name)) return fallback;
  return constrain(server.arg(name).toFloat(), low, high);
}

int boundedInt(const String& name, int fallback, int low, int high) {
  if (!server.hasArg(name)) return fallback;
  return constrain(server.arg(name).toInt(), low, high);
}

void handleSave() {
  if (accessPointMode) {
    server.send(403, "text/plain; charset=utf-8",
                "Die Spritpreis-Konfiguration ist erst nach der WLAN-Verbindung verfügbar.");
    return;
  }

  String requestedPlace = server.arg("place");
  requestedPlace.trim();
  if (requestedPlace.isEmpty()) {
    server.send(400, "text/html; charset=utf-8",
                "<meta charset=utf-8><h2>PLZ oder Ort fehlt</h2><p><a href='/'>Zurück</a></p>");
    return;
  }

  if (requestedPlace != config.locationQuery || isnan(config.latitude) ||
      isnan(config.longitude)) {
    double newLatitude = NAN;
    double newLongitude = NAN;
    String newResolvedLocation;
    String geocodeError;
    if (!geocodeLocation(requestedPlace, newLatitude, newLongitude,
                         newResolvedLocation, geocodeError)) {
      server.send(422, "text/html; charset=utf-8",
                  "<meta charset=utf-8><h2>Ort nicht gespeichert</h2><p>" +
                      htmlEscape(geocodeError) +
                      "</p><p><a href='/'>Eingabe korrigieren</a></p>");
      return;
    }
    config.locationQuery = requestedPlace;
    config.resolvedLocation = newResolvedLocation;
    config.latitude = newLatitude;
    config.longitude = newLongitude;
  }

  config.radiusKm = boundedFloat("radius", config.radiusKm, 1.0f, 25.0f);
  const String fuel = server.arg("fuel");
  if (fuel == "E5" || fuel == "E10" || fuel == "Diesel") config.fuel = fuel;
  config.favoriteId = server.arg("favorite");
  config.alarmPrice = boundedFloat("alarm", config.alarmPrice, 0.0f, 9.0f);
  config.consumption = boundedFloat("consume", config.consumption, 1.0f, 30.0f);
  config.quietStart = boundedInt("quiet_s", config.quietStart, 0, 23);
  config.quietEnd = boundedInt("quiet_e", config.quietEnd, 0, 23);
  config.refreshMinutes = boundedInt("refresh", config.refreshMinutes, 2, 60);
  config.touchXMin = boundedInt("txmin", config.touchXMin, 0, 4095);
  config.touchXMax = boundedInt("txmax", config.touchXMax, 0, 4095);
  config.touchYMin = boundedInt("tymin", config.touchYMin, 0, 4095);
  config.touchYMax = boundedInt("tymax", config.touchYMax, 0, 4095);
  saveConfig();
  fetchStations();
  drawCurrentPage();
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleWifiSave() {
  if (!accessPointMode) {
    server.send(403, "text/plain; charset=utf-8", "WLAN-Einrichtung ist nicht aktiv.");
    return;
  }
  String newSsid = server.arg("ssid");
  newSsid.trim();
  if (newSsid.isEmpty()) {
    server.send(400, "text/html; charset=utf-8",
                "<meta charset=utf-8><h2>Kein WLAN ausgewählt</h2><p><a href='/'>Zurück</a></p>");
    return;
  }
  config.ssid = newSsid;
  config.password = server.arg("password");
  saveConfig();
  server.send(200, "text/html; charset=utf-8",
              "<meta charset=utf-8><meta name=viewport content='width=device-width'>"
              "<h2>WLAN gespeichert</h2><p>Die Spritpreis-Uhr startet neu und verbindet sich. "
              "Öffnen Sie danach die auf dem Display angezeigte IP-Adresse.</p>");
  delay(900);
  ESP.restart();
}

void handleWifiReset() {
  if (accessPointMode) {
    server.sendHeader("Location", "/");
    server.send(303);
    return;
  }
  config.ssid = "";
  config.password = "";
  saveConfig();
  server.send(200, "text/html; charset=utf-8",
              "<meta charset=utf-8><h2>WLAN-Daten gelöscht</h2>"
              "<p>Das Gerät startet mit dem AP <b>Spritpreis-Uhr</b> neu.</p>");
  delay(900);
  ESP.restart();
}

void configureWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/wifi-save", HTTP_POST, handleWifiSave);
  server.on("/wifi-reset", HTTP_POST, handleWifiReset);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/beep", HTTP_POST, [] {
    beep();
    server.sendHeader("Location", "/");
    server.send(303);
  });
  server.on("/refresh", HTTP_POST, [] {
    fetchStations();
    drawCurrentPage();
    server.sendHeader("Location", "/");
    server.send(303);
  });
  // Typische Captive-Portal-Prüfungen von Android, Apple und Windows.
  server.on("/generate_204", HTTP_ANY, handleRoot);
  server.on("/gen_204", HTTP_ANY, handleRoot);
  server.on("/hotspot-detect.html", HTTP_ANY, handleRoot);
  server.on("/library/test/success.html", HTTP_ANY, handleRoot);
  server.on("/connecttest.txt", HTTP_ANY, handleRoot);
  server.on("/ncsi.txt", HTTP_ANY, handleRoot);
  server.onNotFound([] {
    if (accessPointMode) {
      server.sendHeader("Location", deviceUrl(), true);
      server.send(302, "text/plain", "");
    } else {
      server.send(404, "text/plain", "Nicht gefunden");
    }
  });
  server.begin();
}

void connectNetwork() {
  if (!config.ssid.isEmpty()) {
    WiFi.mode(WIFI_STA);
    WiFi.setHostname("spritpreis-uhr");
    WiFi.begin(config.ssid.c_str(), config.password.c_str());
    display.setTextDatum(lgfx::middle_center);
    display.setTextColor(COLOR_TEXT, COLOR_BG);
    display.drawString("Verbinde mit WLAN ...", 120, 125);
    const uint32_t deadline = millis() + 18000;
    while (WiFi.status() != WL_CONNECTED && millis() < deadline) {
      delay(250);
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    accessPointMode = false;
    WiFi.setAutoReconnect(true);
    configTzTime(TZ_EUROPE_BERLIN, "pool.ntp.org", "time.cloudflare.com");
    Serial.println("WLAN verbunden: " + WiFi.localIP().toString());
  } else {
    accessPointMode = true;
    // AP+STA wird benötigt, damit die Einrichtungsseite WLANs scannen kann.
    WiFi.mode(WIFI_AP_STA);
    const String apName = "Spritpreis-Uhr";
    const IPAddress apIp(192, 168, 4, 1);
    const IPAddress netmask(255, 255, 255, 0);
    WiFi.softAPConfig(apIp, apIp, netmask);
    WiFi.softAP(apName.c_str());
    dnsServer.start(53, "*", apIp);
    WiFi.scanNetworks(true, true);
    Serial.println("Einrichtungs-WLAN: " + apName);
    Serial.println("Captive Portal: http://192.168.4.1/");
  }
}

void handleTouch() {
  static bool wasTouched = false;
  static uint32_t lastTouchMs = 0;
  uint16_t x = 0, y = 0;
  const bool touched = display.getTouch(&x, &y);
  if (touched && !wasTouched && millis() - lastTouchMs > 250) {
    lastTouchMs = millis();
    if (x < display.width() / 2) {
      currentPage = (currentPage + PAGE_COUNT - 1) % PAGE_COUNT;
    } else {
      currentPage = (currentPage + 1) % PAGE_COUNT;
    }
    drawCurrentPage();
  }
  wasTouched = touched;
}

void user_setup() {
  pinMode(Pins::SPEAKER, OUTPUT);
  digitalWrite(Pins::SPEAKER, LOW);
  loadConfig();

  display.init();
  uint16_t touchCalibration[8] = {
      static_cast<uint16_t>(config.touchXMin),
      static_cast<uint16_t>(config.touchYMin),
      static_cast<uint16_t>(config.touchXMin),
      static_cast<uint16_t>(config.touchYMax),
      static_cast<uint16_t>(config.touchXMax),
      static_cast<uint16_t>(config.touchYMin),
      static_cast<uint16_t>(config.touchXMax),
      static_cast<uint16_t>(config.touchYMax),
  };
  display.setTouchCalibrate(touchCalibration);
  // Hochformat 240 x 320 in der vom Nutzer gewünschten 270-Grad-Richtung.
  // LovyanGFX-Rotation 2 ist die entgegengesetzte Hochformat-Ausrichtung zu 0.
  display.setRotation(2);
  display.setBrightness(190);
  display.fillScreen(COLOR_BG);
  display.setTextDatum(lgfx::middle_center);
  display.setTextColor(COLOR_ACCENT, COLOR_BG);
  display.setFont(&FONT_DE_BOLD);
  display.drawString("SPRITPREIS-UHR", 120, 115);
  display.setFont(&FONT_DE_NORMAL);
  display.setTextColor(COLOR_MUTED, COLOR_BG);
  display.drawString("Heemol CYD", 120, 151);
  display.setFont(&FONT_DE_SMALL);
  display.drawString("ESP32-2432S028R", 120, 179);

  connectNetwork();
  configureWebServer();
  drawCurrentPage();
  if (WiFi.status() == WL_CONNECTED) {
    fetchStations();
    drawCurrentPage();
  }
}

void user_onTimeChange(int, int) {
  // Die Uhr aktualisiert Zeit und Display in ihrem eigenen Intervall.
}

void user_onBrightnessChange(uint8_t brightness) {
  display.setBrightness(brightness);
}

void user_apBlink(bool) {
  // Der eigene Captive-Portal-Bildschirm ersetzt die AP-Blinkanzeige.
}

void user_loop() {
  if (accessPointMode) dnsServer.processNextRequest();
  server.handleClient();
  handleTouch();

  const uint32_t interval = static_cast<uint32_t>(config.refreshMinutes) * 60000UL;
  const bool locationConfigured = !config.locationQuery.isEmpty() &&
                                  !isnan(config.latitude) &&
                                  !isnan(config.longitude);
  if (WiFi.status() == WL_CONNECTED && locationConfigured && !fetching &&
      (lastFetchMs == 0 || millis() - lastFetchMs >= interval)) {
    fetchStations();
    drawCurrentPage();
  }

  if (millis() - lastScreenRefreshMs >= 30000UL) {
    lastScreenRefreshMs = millis();
    drawCurrentPage();
  }
  delay(5);
}

#endif
