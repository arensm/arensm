#include "DebouncedButton.h"

DebouncedButton::DebouncedButton(uint8_t pin, unsigned long debounceMs)
    : pin_(pin),
      debounceMs_(debounceMs),
      stableState_(HIGH),
      sampledState_(HIGH),
      changedAtMs_(0) {}

void DebouncedButton::begin() {
  pinMode(pin_, INPUT_PULLUP);
  stableState_ = digitalRead(pin_);
  sampledState_ = stableState_;
  changedAtMs_ = millis();
}

bool DebouncedButton::fell() {
  const bool sample = digitalRead(pin_);
  const unsigned long now = millis();

  if (sample != sampledState_) {
    sampledState_ = sample;
    changedAtMs_ = now;
  }

  if (sampledState_ != stableState_ && now - changedAtMs_ >= debounceMs_) {
    const bool previous = stableState_;
    stableState_ = sampledState_;
    return previous == HIGH && stableState_ == LOW;
  }

  return false;
}

