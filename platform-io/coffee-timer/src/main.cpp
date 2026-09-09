#include <Arduino.h>
#include <EEPROM.h>

#include "CountdownTimer.h"
#include "HardwareConfig.h"
#include "InputDevices.h"
#include "TwoDigitDisplay.h"

CountdownTimer timer(HardwareConfig::kMaximumSeconds,
                     HardwareConfig::kCountdownIntervalMs);
TwoDigitDisplay display(HardwareConfig::kShiftDataPin,
                        HardwareConfig::kShiftClockPin,
                        HardwareConfig::kShiftLatchPin);
RotaryEncoder encoder(HardwareConfig::kEncoderAPin,
                      HardwareConfig::kEncoderBPin,
                      HardwareConfig::kEncoderDebounceMs);
DebouncedButton encoderButton(HardwareConfig::kEncoderButtonPin,
                              HardwareConfig::kButtonDebounceMs);

void setRelay(bool enabled) {
  digitalWrite(HardwareConfig::kRelayPin,
               enabled ? HardwareConfig::kRelayOnLevel
                       : HardwareConfig::kRelayOffLevel);
}

void setup() {
  pinMode(HardwareConfig::kRelayPin, OUTPUT);
  setRelay(false);
  encoder.begin();
  encoderButton.begin();

  Serial.begin(HardwareConfig::kSerialBaud);
  Serial.println(F("Coffee timer ready"));

  timer.restore(EEPROM.read(HardwareConfig::kEepromTimerAddress));
  display.show(timer.value());
}

void loop() {
  if (!timer.running()) {
    const int8_t step = encoder.readStep();
    if (timer.adjust(step)) {
      display.show(timer.value());
      EEPROM.update(HardwareConfig::kEepromTimerAddress, timer.value());
      Serial.print(F("Timer set to "));
      Serial.print(timer.value());
      Serial.println(F(" seconds"));
    }
  }

  if (encoderButton.fell()) {
    if (timer.running()) {
      timer.cancel();
      setRelay(false);
      display.show(timer.value());
      Serial.println(F("Countdown cancelled; relay off"));
    } else if (timer.start(millis())) {
      setRelay(true);
      Serial.println(F("Countdown started; relay on"));
    } else {
      Serial.println(F("Set a time greater than zero"));
    }
  }

  const CountdownTimer::TickResult result = timer.update(millis());
  if (result == CountdownTimer::kValueChanged) {
    display.show(timer.value());
    Serial.print(F("Remaining: "));
    Serial.println(timer.value());
  } else if (result == CountdownTimer::kFinished) {
    setRelay(false);
    display.show(0);
    Serial.println(F("Countdown finished; relay off"));
  }
}

