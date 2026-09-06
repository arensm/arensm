#include "InputDevices.h"

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
  const uint8_t sample = digitalRead(pin_);
  const unsigned long now = millis();
  if (sample != sampledState_) {
    sampledState_ = sample;
    changedAtMs_ = now;
  }
  if (sampledState_ != stableState_ && now - changedAtMs_ >= debounceMs_) {
    const uint8_t previous = stableState_;
    stableState_ = sampledState_;
    return previous == HIGH && stableState_ == LOW;
  }
  return false;
}

RotaryEncoder::RotaryEncoder(uint8_t pinA, uint8_t pinB,
                             unsigned long debounceMs)
    : pinA_(pinA),
      pinB_(pinB),
      debounceMs_(debounceMs),
      previousA_(HIGH),
      lastStepAtMs_(0) {}

void RotaryEncoder::begin() {
  pinMode(pinA_, INPUT_PULLUP);
  pinMode(pinB_, INPUT_PULLUP);
  previousA_ = digitalRead(pinA_);
  lastStepAtMs_ = millis();
}

int8_t RotaryEncoder::readStep() {
  const uint8_t currentA = digitalRead(pinA_);
  if (currentA == previousA_) {
    return 0;
  }
  previousA_ = currentA;

  const unsigned long now = millis();
  if (currentA != LOW || now - lastStepAtMs_ < debounceMs_) {
    return 0;
  }
  lastStepAtMs_ = now;
  return digitalRead(pinB_) == HIGH ? 1 : -1;
}

