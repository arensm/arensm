// SPDX-License-Identifier: GPL-3.0-or-later
// Reimplementation based on Carlos Durandal's ATAPI controller Release 3.1.

#include <Arduino.h>
#include <Wire.h>

#include "AtapiBus.h"
#include "CdPlayer.h"
#include "DebouncedButton.h"
#include "HardwareConfig.h"

AtapiBus bus(Wire, HardwareConfig::kDataLowAddress,
             HardwareConfig::kDataHighAddress,
             HardwareConfig::kRegisterSelectAddress);
CdPlayer player(bus, Serial, HardwareConfig::kBusTimeoutMs);

DebouncedButton nextButton(HardwareConfig::kNextPin);
DebouncedButton ejectButton(HardwareConfig::kEjectPin);
DebouncedButton stopButton(HardwareConfig::kStopPin);
DebouncedButton playButton(HardwareConfig::kPlayPin);
DebouncedButton previousButton(HardwareConfig::kPreviousPin);

bool playerReady = false;
unsigned long lastPollAtMs = 0;

void setup() {
  pinMode(HardwareConfig::kLedPin, OUTPUT);
  digitalWrite(HardwareConfig::kLedPin, LOW);

  nextButton.begin();
  ejectButton.begin();
  stopButton.begin();
  playButton.begin();
  previousButton.begin();

  Serial.begin(HardwareConfig::kSerialBaud);
  bus.begin();
  playerReady = player.begin();
  digitalWrite(HardwareConfig::kLedPin, playerReady ? HIGH : LOW);
}

void loop() {
  if (!playerReady) {
    return;
  }

  if (ejectButton.fell()) {
    player.toggleTray();
  }
  if (stopButton.fell()) {
    player.stopPlayback();
  }
  if (playButton.fell()) {
    player.togglePlayPause();
  }
  if (nextButton.fell()) {
    player.nextTrack();
  }
  if (previousButton.fell()) {
    player.previousTrack();
  }

  const unsigned long now = millis();
  if (now - lastPollAtMs >= HardwareConfig::kStatusPollIntervalMs) {
    player.poll();
    lastPollAtMs = now;
  }
}

