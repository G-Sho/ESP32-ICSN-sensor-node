#include "Config.hpp"
#include <ArduinoJson.h>
#include <LittleFS.h>

SystemConfig systemConfig;

/// @brief 16進数文字列をバイト配列に変換する
/// @param hexStr 32文字の16進数文字列（16バイト分）
/// @param out 出力先バッファ（16バイト）
/// @return 成功時true
static bool hexStringToBytes(const char* hexStr, uint8_t* out, size_t outLen) {
  if (hexStr == nullptr || out == nullptr) return false;
  size_t strLen = strlen(hexStr);
  if (strLen != outLen * 2) return false;

  for (size_t i = 0; i < outLen; i++) {
    char byteStr[3] = {hexStr[i * 2], hexStr[i * 2 + 1], '\0'};
    char* endPtr = nullptr;
    unsigned long val = strtoul(byteStr, &endPtr, 16);
    if (endPtr != byteStr + 2 || val > 255) return false;
    out[i] = static_cast<uint8_t>(val);
  }
  return true;
}

bool loadSystemConfig(const char* path) {
  if (!LittleFS.begin()) return false;

  File file = LittleFS.open(path, "r");
  if (!file) return false;

  StaticJsonDocument<512> doc;
  // JsonDocument doc;
  if (deserializeJson(doc, file)) return false;

  systemConfig.maxPitTableSize  = doc["MAX_PIT_TABLE_SIZE"] | 20;
  systemConfig.maxCsTableSize   = doc["MAX_CS_TABLE_SIZE"] | 20;
  systemConfig.cacheEntryTtlUs  = doc["CACHE_ENTRY_TTL_US"] | 100000000; // 100秒
  systemConfig.maxFibTableSize  = doc["MAX_FIB_TABLE_SIZE"] | 20;
  systemConfig.maxVirtualDepth  = doc["MAX_VIRTUAL_DEPTH"] | 5;
  systemConfig.hopCountThreshold = doc["HOP_COUNT_THRESHOLD"] | 10;

  // セキュリティ設定の読み込み
  const char* pmkStr = doc["PMK"] | "";
  const char* lmkStr = doc["LMK"] | "";

  if (strlen(pmkStr) == ESP_NOW_PMK_LEN * 2 && strlen(lmkStr) == ESP_NOW_LMK_LEN * 2) {
    if (hexStringToBytes(pmkStr, systemConfig.pmk, ESP_NOW_PMK_LEN) &&
        hexStringToBytes(lmkStr, systemConfig.lmk, ESP_NOW_LMK_LEN)) {
      systemConfig.encryptionEnabled = true;
    }
  }

  return true;
}
