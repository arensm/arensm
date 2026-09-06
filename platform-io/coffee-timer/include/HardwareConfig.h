#pragma once

#include <Arduino.h>

namespace HardwareConfig {

constexpr uint8_t kShiftClockPin = 2;
constexpr uint8_t kShiftDataPin = 3;
constexpr uint8_t kShiftLatchPin = 4;

constexpr uint8_t kEncoderAPin = 5;
constexpr uint8_t kEncoderBPin = 6;
constexpr uint8_t kEncoderButtonPin = 7;

constexpr uint8_t kRelayPin = 8;
constexpr uint8_t kRelayOnLevel = LOW;
constexpr uint8_t kRelayOffLevel = HIGH;

constexpr uint8_t kMaximumSeconds = 45;
constexpr int kEepromTimerAddress = 0;
constexpr unsigned long kCountdownIntervalMs = 1000;
constexpr unsigned long kButtonDebounceMs = 35;
constexpr unsigned long kEncoderDebounceMs = 5;
constexpr unsigned long kSerialBaud = 9600;

}  // namespace HardwareConfig

