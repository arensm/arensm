#pragma once

#include <Arduino.h>
#include <ShiftRegister74HC595.h>

class TwoDigitDisplay {
 public:
  TwoDigitDisplay(uint8_t dataPin, uint8_t clockPin, uint8_t latchPin);
  void show(uint8_t number);

 private:
  ShiftRegister74HC595<2> shiftRegister_;
};

