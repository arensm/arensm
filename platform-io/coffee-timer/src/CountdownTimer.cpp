#include "CountdownTimer.h"

CountdownTimer::CountdownTimer(uint8_t maximumSeconds,
                               unsigned long intervalMs)
    : maximumSeconds_(maximumSeconds),
      intervalMs_(intervalMs),
      previousTickMs_(0),
      value_(0),
      savedValue_(0),
      running_(false) {}

void CountdownTimer::restore(uint8_t seconds) {
  value_ = seconds <= maximumSeconds_ ? seconds : 0;
  running_ = false;
}

bool CountdownTimer::adjust(int8_t steps) {
  if (running_ || steps == 0) {
    return false;
  }

  int16_t adjusted = static_cast<int16_t>(value_) + steps;
  const int16_t range = static_cast<int16_t>(maximumSeconds_) + 1;
  while (adjusted < 0) {
    adjusted += range;
  }
  while (adjusted >= range) {
    adjusted -= range;
  }
  value_ = static_cast<uint8_t>(adjusted);
  return true;
}

bool CountdownTimer::start(unsigned long nowMs) {
  if (running_ || value_ == 0) {
    return false;
  }
  savedValue_ = value_;
  running_ = true;
  previousTickMs_ = nowMs;
  return true;
}

void CountdownTimer::cancel() {
  running_ = false;
  value_ = savedValue_;
}

CountdownTimer::TickResult CountdownTimer::update(unsigned long nowMs) {
  if (!running_ || nowMs - previousTickMs_ < intervalMs_) {
    return kNoChange;
  }

  const unsigned long elapsedIntervals =
      (nowMs - previousTickMs_) / intervalMs_;
  previousTickMs_ += elapsedIntervals * intervalMs_;
  if (elapsedIntervals >= value_) {
    value_ = 0;
    running_ = false;
    return kFinished;
  }

  value_ -= static_cast<uint8_t>(elapsedIntervals);
  return kValueChanged;
}

uint8_t CountdownTimer::value() const { return value_; }

bool CountdownTimer::running() const { return running_; }

