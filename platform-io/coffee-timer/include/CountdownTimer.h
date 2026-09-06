#pragma once

#include <Arduino.h>

class CountdownTimer {
 public:
  enum TickResult : uint8_t {
    kNoChange,
    kValueChanged,
    kFinished,
  };

  CountdownTimer(uint8_t maximumSeconds, unsigned long intervalMs);

  void restore(uint8_t seconds);
  bool adjust(int8_t steps);
  bool start(unsigned long nowMs);
  void cancel();
  TickResult update(unsigned long nowMs);

  uint8_t value() const;
  bool running() const;

 private:
  uint8_t maximumSeconds_;
  unsigned long intervalMs_;
  unsigned long previousTickMs_;
  uint8_t value_;
  bool running_;
};

