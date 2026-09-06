#pragma once

#include <Arduino.h>

namespace HardwareConfig {

constexpr uint8_t kLedPin = LED_BUILTIN;
constexpr uint8_t kNextPin = 12;
constexpr uint8_t kEjectPin = 11;
constexpr uint8_t kStopPin = 10;
constexpr uint8_t kPlayPin = 9;
constexpr uint8_t kPreviousPin = 8;

constexpr uint8_t kDataLowAddress = 0x20;
constexpr uint8_t kDataHighAddress = 0x21;
constexpr uint8_t kRegisterSelectAddress = 0x22;

constexpr unsigned long kSerialBaud = 9600;
constexpr unsigned long kStatusPollIntervalMs = 250;
constexpr unsigned long kBusTimeoutMs = 5000;

}  // namespace HardwareConfig

