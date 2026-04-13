#pragma once

#include <cstddef>
#include <cstdint>

// コンパイル時定数の定義
constexpr size_t MAX_PIT_TABLE_SIZE = 20;
constexpr size_t MAX_CS_TABLE_SIZE = 20;
constexpr size_t MAX_FIB_TABLE_SIZE = 20;

// セキュリティ関連定数
constexpr size_t ESP_NOW_PMK_LEN = 16;
constexpr size_t ESP_NOW_LMK_LEN = 16;

struct SystemConfig {
  size_t maxPitTableSize = MAX_PIT_TABLE_SIZE;
  size_t maxCsTableSize = MAX_CS_TABLE_SIZE;
  uint32_t cacheEntryTtlUs = 100000000; // 100秒
  size_t maxFibTableSize = MAX_FIB_TABLE_SIZE;
  int maxVirtualDepth = 5;
  int hopCountThreshold = 10;

  // セキュリティ設定
  uint8_t pmk[ESP_NOW_PMK_LEN] = {0};  // Primary Master Key
  uint8_t lmk[ESP_NOW_LMK_LEN] = {0};  // Local Master Key
  bool encryptionEnabled = false;
};

extern SystemConfig systemConfig;

bool loadSystemConfig(const char* path = "/config.json");