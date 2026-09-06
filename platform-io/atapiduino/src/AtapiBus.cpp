#include "AtapiBus.h"

namespace {
constexpr uint8_t kBusyBit = _BV(7);
constexpr uint8_t kDriveReadyBit = _BV(6);
constexpr uint8_t kDataRequestBit = _BV(3);
}  // namespace

AtapiBus::AtapiBus(TwoWire& wire, uint8_t dataLowAddress,
                   uint8_t dataHighAddress, uint8_t registerSelectAddress)
    : wire_(wire),
      dataLowAddress_(dataLowAddress),
      dataHighAddress_(dataHighAddress),
      registerSelectAddress_(registerSelectAddress) {}

void AtapiBus::begin() {
  wire_.begin();
  release();
}

bool AtapiBus::reset() {
  writeExpander(registerSelectAddress_, B11011111);
  delay(40);
  writeExpander(registerSelectAddress_, B11111111);
  delay(20);
  return read(kAlternateStatusControl).low != 0xFF;
}

AtapiBus::Word AtapiBus::read(Register address) {
  writeExpander(registerSelectAddress_,
                static_cast<uint8_t>(address) & B01111111);
  // Preserve the access order used by the proven circuit: high byte first.
  const uint8_t high = readExpander(dataHighAddress_);
  const uint8_t low = readExpander(dataLowAddress_);
  const Word result = {low, high};
  release();
  return result;
}

void AtapiBus::write(Register address, uint8_t low, uint8_t high) {
  const uint8_t selector = static_cast<uint8_t>(address);
  writeExpander(registerSelectAddress_, selector | B01000000);
  writeExpander(dataHighAddress_, high);
  writeExpander(dataLowAddress_, low);
  writeExpander(registerSelectAddress_, selector & B10111111);
  release();
}

void AtapiBus::release() {
  writeExpander(registerSelectAddress_, 0xFF);
  writeExpander(dataHighAddress_, 0xFF);
  writeExpander(dataLowAddress_, 0xFF);
}

bool AtapiBus::waitBusyClear(unsigned long timeoutMs) {
  return waitForStatus(kBusyBit, false, timeoutMs);
}

bool AtapiBus::waitDataRequest(bool requested, unsigned long timeoutMs) {
  return waitForStatus(kDataRequestBit, requested, timeoutMs);
}

bool AtapiBus::waitDriveReady(unsigned long timeoutMs) {
  return waitForStatus(kDriveReadyBit, true, timeoutMs);
}

bool AtapiBus::waitForStatus(uint8_t mask, bool set,
                             unsigned long timeoutMs) {
  const unsigned long startedAt = millis();
  do {
    const bool bitIsSet = (read(kAlternateStatusControl).low & mask) != 0;
    if (bitIsSet == set) {
      return true;
    }
  } while (millis() - startedAt < timeoutMs);
  return false;
}

void AtapiBus::writeExpander(uint8_t address, uint8_t value) {
  wire_.beginTransmission(address);
  wire_.write(value);
  wire_.endTransmission();
}

uint8_t AtapiBus::readExpander(uint8_t address) {
  wire_.requestFrom(static_cast<int>(address), 1);
  return wire_.available() ? wire_.read() : 0xFF;
}
