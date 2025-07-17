#include "Config.hpp"
#include <ArduinoJson.h>
#include <LittleFS.h>

SystemConfig systemConfig;

bool loadSystemConfig(const char* path) {
  if (!LittleFS.begin()) return false;

  File file = LittleFS.open(path, "r");
  if (!file) return false;

  // StaticJsonDocument<512> doc;
  JsonDocument doc;
  if (deserializeJson(doc, file)) return false;

  systemConfig.maxPitTableSize  = doc["MAX_PIT_TABLE_SIZE"] | 20;
  systemConfig.maxCsTableSize   = doc["MAX_CS_TABLE_SIZE"] | 20;
  systemConfig.cacheEntryTtlUs  = doc["CACHE_ENTRY_TTL_US"] | 1000000.0;
  systemConfig.maxFibTableSize  = doc["MAX_FIB_TABLE_SIZE"] | 20;
  systemConfig.maxVirtualDepth  = doc["MAX_VIRTUAL_DEPTH"] | 5;
  systemConfig.hopCountThreshold = doc["HOP_COUNT_THRESHOLD"] | 10;

  return true;
}
