#pragma once

#include <Arduino.h>

// Einheitliche Schnittstelle, die jede Uhr in ihrer cl_*.cpp implementiert.
void user_setup();
void user_onTimeChange(int hour, int minute);
void user_onBrightnessChange(uint8_t brightness);
void user_apBlink(bool on);
void user_loop();
