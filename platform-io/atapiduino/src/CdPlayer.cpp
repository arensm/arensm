#include "CdPlayer.h"

#include <string.h>

namespace {
constexpr uint8_t kDiscPresent = 0x00;
constexpr uint8_t kNoDisc = 0xFF;
constexpr uint8_t kTrayOpen = 0x71;
constexpr uint8_t kMaximumResponseWords = 255;

void clearPacket(uint8_t* packet) { memset(packet, 0, 16); }
}  // namespace

CdPlayer::CdPlayer(AtapiBus& bus, Print& output,
                   unsigned long busTimeoutMs)
    : bus_(bus),
      output_(output),
      busTimeoutMs_(busTimeoutMs),
      packetLength_(12),
      firstTrack_(1),
      lastTrack_(1),
      currentTrack_(1),
      playStart_{0, 0, 0},
      discEnd_{0, 0, 0},
      position_{0, 0, 0},
      audioStatus_(kUnknown),
      tocValid_(false) {}

bool CdPlayer::begin() {
  output_.println(F("Atapiduino"));
  output_.println(F("PlatformIO rewrite 1.0"));
  return initializeDevice();
}

bool CdPlayer::initializeDevice() {
  if (!bus_.reset()) {
    output_.println(F("ERROR: IDE device did not answer reset"));
    return false;
  }

  delay(5000);
  if (!bus_.waitBusyClear(busTimeoutMs_) ||
      !bus_.waitDriveReady(busTimeoutMs_)) {
    output_.println(F("ERROR: IDE device not ready"));
    return false;
  }

  const uint8_t signatureLow = bus_.read(AtapiBus::kCylinderLow).low;
  const uint8_t signatureHigh = bus_.read(AtapiBus::kCylinderHigh).low;
  const bool validLow = signatureLow == 0x14 || signatureLow == 0x69;
  const bool validHigh = signatureHigh == 0xEB || signatureHigh == 0x96;
  if (!validLow || !validHigh) {
    output_.print(F("ERROR: invalid ATAPI signature 0x"));
    output_.print(signatureHigh, HEX);
    output_.println(signatureLow, HEX);
    return false;
  }
  output_.println(F("Found ATAPI device"));

  bus_.write(AtapiBus::kDeviceHead, 0x00);
  initializeTaskFile();

  delay(3000);
  bus_.write(AtapiBus::kCommandStatus, 0x90);
  bus_.waitBusyClear(busTimeoutMs_);
  const bool diagnosticsPassed =
      bus_.read(AtapiBus::kErrorFeature).low == 0x01;
  output_.println(diagnosticsPassed ? F("Self diagnostic: OK")
                                    : F("Self diagnostic: warning"));

  if (!identifyDevice()) {
    return false;
  }
  return waitUntilReady();
}

bool CdPlayer::identifyDevice() {
  output_.print(F("ATAPI device: "));
  bus_.write(AtapiBus::kCommandStatus, 0xA1);
  if (!bus_.waitDataRequest(true, busTimeoutMs_)) {
    output_.println(F("identify timeout"));
    return false;
  }

  uint8_t wordIndex = 0;
  do {
    const AtapiBus::Word word = bus_.read(AtapiBus::kData);
    if (wordIndex == 0 && (word.low & 0x01) != 0) {
      packetLength_ = 16;
    }
    if (wordIndex >= 27 && wordIndex <= 46) {
      output_.write(word.high);
      output_.write(word.low);
    }
    ++wordIndex;
  } while (wordIndex != 0 &&
           (bus_.read(AtapiBus::kCommandStatus).low & _BV(3)) != 0);

  output_.println();
  if (!bus_.waitDataRequest(false, busTimeoutMs_)) {
    output_.println(F("ERROR: identify data phase did not finish"));
    return false;
  }
  return true;
}

void CdPlayer::initializeTaskFile() {
  bus_.write(AtapiBus::kErrorFeature, 0x00);
  bus_.write(AtapiBus::kCylinderHigh, 0x02);
  bus_.write(AtapiBus::kCylinderLow, 0x00);
  bus_.write(AtapiBus::kAlternateStatusControl, 0x02);
  bus_.waitBusyClear(busTimeoutMs_);
  bus_.waitDataRequest(false, busTimeoutMs_);
}

bool CdPlayer::testUnitReady() {
  uint8_t packet[16];
  clearPacket(packet);
  return sendPacket(packet);
}

uint8_t CdPlayer::requestSense() {
  uint8_t packet[16];
  clearPacket(packet);
  packet[0] = 0x03;
  packet[4] = 0xFF;
  if (!sendPacket(packet) ||
      !bus_.waitDataRequest(true, busTimeoutMs_)) {
    return 0xFF;
  }

  uint8_t additionalSenseCode = 0xFF;
  uint8_t wordIndex = 0;
  do {
    const AtapiBus::Word word = bus_.read(AtapiBus::kData);
    if (wordIndex == 6) {
      additionalSenseCode = word.low;
    }
    ++wordIndex;
  } while (wordIndex < kMaximumResponseWords &&
           (bus_.read(AtapiBus::kCommandStatus).low & _BV(3)) != 0);
  return additionalSenseCode;
}

bool CdPlayer::waitUntilReady() {
  const unsigned long startedAt = millis();
  do {
    testUnitReady();
    const uint8_t sense = requestSense();
    if (sense != 0x04 && sense != 0x29) {
      return true;
    }
    delay(100);
  } while (millis() - startedAt < 30000UL);

  output_.println(F("ERROR: drive stayed unavailable"));
  return false;
}

bool CdPlayer::sendPacket(const uint8_t* packet) {
  bus_.write(AtapiBus::kAlternateStatusControl, B00001010);
  bus_.write(AtapiBus::kCommandStatus, 0xA0);
  if (!bus_.waitDataRequest(true, busTimeoutMs_)) {
    output_.println(F("ERROR: packet request timeout"));
    return false;
  }

  for (uint8_t index = 0; index < packetLength_; index += 2) {
    bus_.write(AtapiBus::kData, packet[index], packet[index + 1]);
    bus_.read(AtapiBus::kAlternateStatusControl);
    bus_.read(AtapiBus::kAlternateStatusControl);
  }
  return bus_.waitBusyClear(busTimeoutMs_);
}

void CdPlayer::toggleTray() {
  tocValid_ = false;
  const uint8_t discState = checkDisc();
  if (discState == kTrayOpen) {
    output_.println(F("LOAD"));
    startStopUnit(0x03);
  } else {
    output_.println(F("OPEN"));
    startStopUnit(0x02);
  }
  currentTrack_ = firstTrack_;
}

void CdPlayer::stopPlayback() {
  currentTrack_ = firstTrack_;
  uint8_t packet[16];
  clearPacket(packet);
  packet[0] = 0x4E;
  sendPacket(packet);
  startStopUnit(0x00);
  tocValid_ = false;
}

void CdPlayer::togglePlayPause() {
  if (audioStatus_ == kPlaying) {
    pause();
  } else if (audioStatus_ == kPaused) {
    resume();
  } else {
    play();
  }
  tocValid_ = false;
}

void CdPlayer::nextTrack() {
  if (!tocValid_ && !readToc()) {
    return;
  }
  const bool keepPaused = audioStatus_ == kPaused || audioStatus_ == kStopped;
  const uint8_t next = currentTrack_ >= lastTrack_ ? firstTrack_
                                                   : currentTrack_ + 1;
  selectTrack(next);
  play();
  if (keepPaused) {
    pause();
  }
}

void CdPlayer::previousTrack() {
  if (!tocValid_ && !readToc()) {
    return;
  }
  const bool keepPaused = audioStatus_ == kPaused || audioStatus_ == kStopped;
  const uint8_t previous = currentTrack_ <= firstTrack_ ? lastTrack_
                                                        : currentTrack_ - 1;
  selectTrack(previous);
  play();
  if (keepPaused) {
    pause();
  }
}

void CdPlayer::poll() {
  readSubchannel();
  if (audioStatus_ == kPlaying) {
    output_.print(F("PLAY "));
    printPosition();
  } else if (audioStatus_ == kPaused) {
    output_.print(F("PAUSE "));
    printPosition();
  } else if (audioStatus_ == kStopped && !tocValid_) {
    if (readToc()) {
      printDiscSummary();
    }
  } else if (audioStatus_ == kUnknown) {
    output_.println(F("NO DISC"));
  }
}

bool CdPlayer::readToc() {
  uint8_t packet[16];
  clearPacket(packet);
  packet[0] = 0x43;
  packet[1] = 0x02;
  packet[7] = 0xFF;
  packet[8] = 0xFF;
  if (!sendPacket(packet) ||
      !bus_.waitDataRequest(true, busTimeoutMs_)) {
    return false;
  }

  bus_.read(AtapiBus::kData);
  const AtapiBus::Word trackRange = bus_.read(AtapiBus::kData);
  firstTrack_ = trackRange.low;
  lastTrack_ = trackRange.high;
  if (currentTrack_ < firstTrack_ || currentTrack_ > lastTrack_) {
    currentTrack_ = firstTrack_;
  }

  uint8_t descriptorCount = 0;
  while ((bus_.read(AtapiBus::kCommandStatus).low & _BV(3)) != 0 &&
         descriptorCount < 100) {
    bus_.read(AtapiBus::kData);
    const uint8_t track = bus_.read(AtapiBus::kData).low;
    const AtapiBus::Word minuteWord = bus_.read(AtapiBus::kData);
    const AtapiBus::Word secondFrameWord = bus_.read(AtapiBus::kData);
    const Msf address = {minuteWord.high, secondFrameWord.low,
                         secondFrameWord.high};
    if (track == firstTrack_) {
      playStart_ = address;
    }
    if (track == currentTrack_) {
      playStart_ = address;
    }
    if (track == 0xAA) {
      discEnd_ = address;
    }
    ++descriptorCount;
  }

  tocValid_ = descriptorCount > 0;
  return tocValid_;
}

void CdPlayer::readSubchannel() {
  uint8_t packet[16];
  clearPacket(packet);
  packet[0] = 0x42;
  packet[1] = 0x02;
  packet[2] = 0x40;
  packet[3] = 0x01;
  packet[7] = 0xFF;
  packet[8] = 0xFF;
  if (!sendPacket(packet) ||
      !bus_.waitDataRequest(true, busTimeoutMs_)) {
    audioStatus_ = kUnknown;
    return;
  }

  uint8_t status = bus_.read(AtapiBus::kData).high;
  if (status == 0x13) {
    status = kStopped;
  }
  if (status == kPlaying || status == kPaused || status == kStopped) {
    audioStatus_ = static_cast<AudioStatus>(status);
  } else {
    audioStatus_ = kUnknown;
  }

  bus_.read(AtapiBus::kData);
  bus_.read(AtapiBus::kData);
  currentTrack_ = bus_.read(AtapiBus::kData).low;
  const AtapiBus::Word minuteWord = bus_.read(AtapiBus::kData);
  const AtapiBus::Word secondFrameWord = bus_.read(AtapiBus::kData);
  position_ = {minuteWord.low, minuteWord.high, secondFrameWord.low};
  drainDataPhase();
}

uint8_t CdPlayer::checkDisc() {
  uint8_t packet[16];
  clearPacket(packet);
  packet[0] = 0x5A;
  packet[2] = 0x01;
  packet[7] = 0xFF;
  packet[8] = 0xFF;
  if (!sendPacket(packet) ||
      !bus_.waitDataRequest(true, busTimeoutMs_)) {
    return kNoDisc;
  }

  bus_.read(AtapiBus::kData);
  const uint8_t mediumType = bus_.read(AtapiBus::kData).low;
  drainDataPhase();
  if (mediumType == kTrayOpen) {
    return kTrayOpen;
  }
  switch (mediumType) {
    case 0x02:
    case 0x06:
    case 0x12:
    case 0x16:
    case 0x22:
    case 0x26:
      return kDiscPresent;
    default:
      return kNoDisc;
  }
}

void CdPlayer::drainDataPhase() {
  uint8_t words = 0;
  while ((bus_.read(AtapiBus::kCommandStatus).low & _BV(3)) != 0 &&
         words < kMaximumResponseWords) {
    bus_.read(AtapiBus::kData);
    ++words;
  }
}

void CdPlayer::play() {
  uint8_t packet[16];
  clearPacket(packet);
  packet[0] = 0x47;
  packet[3] = playStart_.minute;
  packet[4] = playStart_.second;
  packet[5] = playStart_.frame;
  packet[6] = discEnd_.minute;
  packet[7] = discEnd_.second;
  packet[8] = discEnd_.frame;
  sendPacket(packet);
}

void CdPlayer::pause() {
  uint8_t packet[16];
  clearPacket(packet);
  packet[0] = 0x4B;
  sendPacket(packet);
}

void CdPlayer::resume() {
  uint8_t packet[16];
  clearPacket(packet);
  packet[0] = 0x4B;
  packet[8] = 0x01;
  sendPacket(packet);
}

void CdPlayer::startStopUnit(uint8_t action) {
  uint8_t packet[16];
  clearPacket(packet);
  packet[0] = 0x1B;
  packet[4] = action;
  sendPacket(packet);
}

void CdPlayer::selectTrack(uint8_t track) {
  currentTrack_ = track;
  readToc();
}

void CdPlayer::printDiscSummary() const {
  output_.print(F("Tracks "));
  output_.print(firstTrack_);
  output_.print('-');
  output_.println(lastTrack_);
  output_.print(F("Time "));
  output_.print(discEnd_.minute);
  output_.print(':');
  if (discEnd_.second < 10) {
    output_.print('0');
  }
  output_.println(discEnd_.second);
}

void CdPlayer::printPosition() const {
  output_.print(F("track "));
  output_.print(currentTrack_);
  output_.print(' ');
  output_.print(position_.minute);
  output_.print(':');
  if (position_.second < 10) {
    output_.print('0');
  }
  output_.println(position_.second);
}

