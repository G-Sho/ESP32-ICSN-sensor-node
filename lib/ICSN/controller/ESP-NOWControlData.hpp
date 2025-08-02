#pragma once

#include <Arduino.h>
#include <array>

#define MAX_TX_ADDRESSES 20 // Maximum number of addresses to send to
#define MAX_SIGNAL_CODE_LENGTH 10 // Maximum length of signal code
#define MAX_CONTENT_NAME_LENGTH 100 // Maximum length of content name
#define MAX_CONTENT_LENGTH 20 // Maximum length of content

/// @brief Data structure for ICSN Application
struct ESP_NOWControlData {
  std::array<std::array<uint8_t, 6>, 20> txAddress; // 複数送信先対応
  char signalCode[MAX_SIGNAL_CODE_LENGTH];
  uint8_t hopCount;
  char contentName[MAX_CONTENT_NAME_LENGTH];
  char content[MAX_CONTENT_LENGTH];
};


/// @brief Data structure for ESP-NOW communication
struct CommunicationData
{
  char signalCode[MAX_SIGNAL_CODE_LENGTH];
  uint8_t hopCount;
  char contentName[MAX_CONTENT_NAME_LENGTH];
  char content[MAX_CONTENT_LENGTH];
};