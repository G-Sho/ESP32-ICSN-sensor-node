#pragma once

#include <cstddef>

struct SystemConfig {
  size_t maxPitTableSize = 20;
  size_t maxCsTableSize = 20;
  double cacheEntryTtlUs = 1000000.0;
  size_t maxFibTableSize = 20;
  int maxVirtualDepth = 5;
  int hopCountThreshold = 10;
};

extern SystemConfig systemConfig;

bool loadSystemConfig(const char* path = "/config.json");