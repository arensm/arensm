#pragma once

#include <Arduino.h>

#include "AtapiBus.h"

class CdPlayer {
 public:
  CdPlayer(AtapiBus& bus, Print& output, unsigned long busTimeoutMs);

  bool begin();
  void poll();

  void toggleTray();
  void stopPlayback();
  void togglePlayPause();
  void nextTrack();
  void previousTrack();

 private:
  enum AudioStatus : uint8_t {
    kUnknown = 0x00,
    kPlaying = 0x11,
    kPaused = 0x12,
    kStopped = 0x15,
  };

  struct Msf {
    uint8_t minute;
    uint8_t second;
    uint8_t frame;
  };

  bool initializeDevice();
  bool identifyDevice();
  void initializeTaskFile();
  bool testUnitReady();
  uint8_t requestSense();
  bool waitUntilReady();

  bool sendPacket(const uint8_t* packet);
  bool readToc();
  void readSubchannel();
  uint8_t checkDisc();
  void drainDataPhase();

  void play();
  void pause();
  void resume();
  void startStopUnit(uint8_t action);
  void selectTrack(uint8_t track);
  void printDiscSummary() const;
  void printPosition() const;

  AtapiBus& bus_;
  Print& output_;
  unsigned long busTimeoutMs_;
  uint8_t packetLength_;
  uint8_t firstTrack_;
  uint8_t lastTrack_;
  uint8_t currentTrack_;
  Msf playStart_;
  Msf discEnd_;
  Msf position_;
  AudioStatus audioStatus_;
  bool tocValid_;
};

