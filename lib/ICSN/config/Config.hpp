#pragma once

#include <cstddef>
#include <cstdint>

struct SystemConfig {
  size_t maxPitTableSize = 20;
  size_t maxCsTableSize = 20;
  uint32_t cacheEntryTtlUs = 100000000; // 100 second
  size_t maxFibTableSize = 20;
  int maxVirtualDepth = 5;
  int hopCountThreshold = 10;
};

extern SystemConfig systemConfig;

bool loadSystemConfig(const char* path = "/config.json");