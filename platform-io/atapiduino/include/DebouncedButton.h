#pragma once

#include <Arduino.h>

class DebouncedButton {
 public:
  explicit DebouncedButton(uint8_t pin, unsigned long debounceMs = 35);

  void begin();
  bool fell();

 private:
  uint8_t pin_;
  unsigned long debounceMs_;
  bool stableState_;
  bool sampledState_;
  unsigned long changedAtMs_;
};

