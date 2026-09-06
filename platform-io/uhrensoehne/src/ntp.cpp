#include "ntp.h"
#include "config.h"
#include <time.h>
// ============================================================
//  ntp.cpp  –  NTP-Sync + millis()-basierte Zeitberechnung
//              + täglicher Resync um NTP_RESYNC_HOUR1/2
// ============================================================

uint8_t gHour   = 0;
uint8_t gMinute = 0;
uint8_t gSecond = 0;

// Interner Referenzpunkt
static uint8_t       _refHour   = 0;
static uint8_t       _refMinute = 0;
static uint8_t       _refSecond = 0;
static unsigned long _refMillis = 0;

// Resync-Tracking
static int  _lastResyncDay = -1;
static bool _resyncedAt1   = false;
static bool _resyncedAt2   = false;

// ---- Internes Sync ------------------------------------------
static void _doSync() {
#ifdef ESP32
  configTime(0, 0, NTP_SERVER1, NTP_SERVER2);
  setenv("TZ", NTP_TZ, 1);
  tzset();
#else
  configTime(NTP_TZ, NTP_SERVER1, NTP_SERVER2);
#endif
  Serial.print("[NTP] Sync");
  time_t n = time(nullptr);
  uint32_t t0 = millis();
  while (n < (time_t)(8UL * 3600 * 2) && (millis() - t0) < 10000UL) {
    delay(200); n = time(nullptr); Serial.print(".");
  }
  Serial.println();
  struct tm tm_; localtime_r(&n, &tm_);
  _refHour   = tm_.tm_hour;
  _refMinute = tm_.tm_min;
  _refSecond = tm_.tm_sec;
  _refMillis = millis();
  _lastResyncDay = tm_.tm_yday;
  _resyncedAt1 = _resyncedAt2 = false;
  Serial.printf("[NTP] OK %02d:%02d:%02d\n", _refHour, _refMinute, _refSecond);
}

// ---- Public -------------------------------------------------
void ntpSetup() {
  _doSync();
}

void ntpForceSync() {
  _doSync();
}

void ntpUpdate() {
  // Zeit aus millis() berechnen
  unsigned long elapsed = millis() - _refMillis;
  unsigned long totalSec = _refSecond + elapsed / 1000UL;
  unsigned long totalMin = _refMinute + totalSec / 60UL;
  gSecond = (uint8_t)(totalSec % 60);
  gHour   = (uint8_t)((_refHour + totalMin / 60UL) % 24);
  gMinute = (uint8_t)(totalMin % 60);

  // Täglicher Resync
  time_t n = time(nullptr); struct tm tm_; localtime_r(&n, &tm_);
  if (tm_.tm_yday != _lastResyncDay) {
    _lastResyncDay = tm_.tm_yday;
    _resyncedAt1 = _resyncedAt2 = false;
  }
  if (tm_.tm_min == 0) {
    if (tm_.tm_hour == NTP_RESYNC_HOUR1 && !_resyncedAt1) { _doSync(); _resyncedAt1 = true; }
    if (tm_.tm_hour == NTP_RESYNC_HOUR2 && !_resyncedAt2) { _doSync(); _resyncedAt2 = true; }
  }
}

uint16_t currentMinutesSinceMidnight() {
  return (uint16_t)(gHour * 60 + gMinute);
}

String uptimeString() {
  unsigned long t = millis() / 1000UL;
  unsigned int d = t / 86400; t %= 86400;
  unsigned int h = t / 3600;  t %= 3600;
  unsigned int m = t / 60;    unsigned int s = t % 60;
  char buf[32];
  snprintf(buf, sizeof(buf), "%ud %02u:%02u:%02u", d, h, m, s);
  return String(buf);
}

bool parseHHMM(const String& s, uint16_t& out) {
  int p = s.indexOf(':');
  if (p < 0) return false;
  int hh = s.substring(0, p).toInt();
  int mm = s.substring(p + 1).toInt();
  if (hh < 0 || hh > 23 || mm < 0 || mm > 59) return false;
  out = (uint16_t)(hh * 60 + mm);
  return true;
}

String formatHHMM(uint16_t mins) {
  char buf[8];
  snprintf(buf, sizeof(buf), "%02d:%02d", mins / 60, mins % 60);
  return String(buf);
}
