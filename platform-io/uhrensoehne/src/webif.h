#pragma once
#include <Arduino.h>
#ifdef ESP32
  #include <WebServer.h>
  #include <HTTPUpdateServer.h>
  extern WebServer webServer;
#else
  #include <ESP8266WebServer.h>
  #include <ESP8266HTTPUpdateServer.h>
  extern ESP8266WebServer webServer;
#endif
// ============================================================
//  webif.h  –  Webserver (Hauptseite + Debug-Seite)
// ============================================================

void webifSetup();
void webifLoop();

void user_onTimeChange(int h, int m);
void user_onBrightnessChange(uint8_t brightness);
bool user_ledOn(int idx);
void user_onColorsChange();
void user_allLedsOn();
void user_allLedsOff();
String user_getActiveLeds();
String user_timeToWords(int h, int m);
