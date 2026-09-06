#pragma once

#include <Arduino.h>
#include <Wire.h>

class AtapiBus {
 public:
  struct Word {
    uint8_t low;
    uint8_t high;
  };

  enum Register : uint8_t {
    kData = 0xF0,
    kErrorFeature = 0xF1,
    kSectorCount = 0xF2,
    kSectorNumber = 0xF3,
    kCylinderLow = 0xF4,
    kCylinderHigh = 0xF5,
    kDeviceHead = 0xF6,
    kCommandStatus = 0xF7,
    kAlternateStatusControl = 0xEE,
  };

  AtapiBus(TwoWire& wire, uint8_t dataLowAddress, uint8_t dataHighAddress,
           uint8_t registerSelectAddress);

  void begin();
  bool reset();
  Word read(Register address);
  void write(Register address, uint8_t low, uint8_t high = 0xFF);
  void release();

  bool waitBusyClear(unsigned long timeoutMs);
  bool waitDataRequest(bool requested, unsigned long timeoutMs);
  bool waitDriveReady(unsigned long timeoutMs);

 private:
  bool waitForStatus(uint8_t mask, bool set, unsigned long timeoutMs);
  void writeExpander(uint8_t address, uint8_t value);
  uint8_t readExpander(uint8_t address);

  TwoWire& wire_;
  uint8_t dataLowAddress_;
  uint8_t dataHighAddress_;
  uint8_t registerSelectAddress_;
};

