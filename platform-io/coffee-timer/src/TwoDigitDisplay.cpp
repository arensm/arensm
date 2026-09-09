#include "TwoDigitDisplay.h"

namespace {
// Active-low segment patterns retained from the proven circuit.
constexpr uint8_t kDigitPatterns[] = {
    B11000000, B11111001, B10100100, B10110000, B10011001,
    B10010010, B10000011, B11111000, B10000000, B10011000,
};
}  // namespace

TwoDigitDisplay::TwoDigitDisplay(uint8_t dataPin, uint8_t clockPin,
                                 uint8_t latchPin)
    : shiftRegister_(dataPin, clockPin, latchPin) {}

void TwoDigitDisplay::show(uint8_t number) {
  if (number > 99) number = 99;
  const uint8_t digits[] = {kDigitPatterns[number / 10],
                            kDigitPatterns[number % 10]};
  shiftRegister_.setAll(digits);
}

