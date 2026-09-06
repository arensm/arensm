#pragma once

#include <Arduino.h>

class DebouncedButton {
 public:
  DebouncedButton(uint8_t pin, unsigned long debounceMs);

  void begin();
  bool fell();

 private:
  uint8_t pin_;
  unsigned long debounceMs_;
  uint8_t stableState_;
  uint8_t sampledState_;
  unsigned long changedAtMs_;
};

class RotaryEncoder {
 public:
  RotaryEncoder(uint8_t pinA, uint8_t pinB, unsigned long debounceMs);

  void begin();
  int8_t readStep();

 private:
  uint8_t pinA_;
  uint8_t pinB_;
  unsigned long debounceMs_;
  uint8_t previousA_;
  unsigned long lastStepAtMs_;
};

