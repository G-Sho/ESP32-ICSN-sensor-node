#pragma once

#include <Arduino.h>
#include <array>

struct ESP_NOWControlData {
  std::array<std::array<uint8_t, 6>, 20> txAddress; // 複数送信先対応
  String signalCode;
  uint8_t hopCount;
  String contentName;
  String content;
};

/// @brief It is used in the ESP-NOW communication
struct CommunicationData
{
  String signalCode;    // SIGNAL_INTEREST, SIGNAL_DATA
  uint8_t hopCount = 0; // Hop count for Interest packet
  String contentName;
  String content;
};