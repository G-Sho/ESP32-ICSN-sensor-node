#pragma once

#include <cstddef>
#include <cstdint>

// コンパイル時定数の定義
constexpr size_t MAX_PIT_TABLE_SIZE = 20;
constexpr size_t MAX_CS_TABLE_SIZE = 20;
constexpr size_t MAX_FIB_TABLE_SIZE = 20;

struct SystemConfig {
  size_t maxPitTableSize = MAX_PIT_TABLE_SIZE;
  size_t maxCsTableSize = MAX_CS_TABLE_SIZE;
  uint32_t cacheEntryTtlUs = 100000000; // 100秒
  size_t maxFibTableSize = MAX_FIB_TABLE_SIZE;
  int maxVirtualDepth = 5;
  int hopCountThreshold = 10;
};

extern SystemConfig systemConfig;

bool loadSystemConfig(const char* path = "/config.json");