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

/// @brief ピア固有LMK設定エントリ
struct PeerLMKConfig {
  uint8_t mac[6];               ///< ピアのMACアドレス
  uint8_t lmk[ESP_NOW_LMK_LEN]; ///< このピア向けのLocal Master Key
  bool    valid;                 ///< エントリが有効かどうか
};

/// @brief ピア固有LMKの最大登録数
constexpr size_t MAX_PEER_LMK_ENTRIES = 20;

/// @brief FIB初期エントリ（起動時にFIBへ投入するルーティング設定）
struct FibInitEntry {
  char contentName[64];  ///< コンテンツ名プレフィックス（例: "/iot/buildingA/room101"）
  char nextHopMac[18];   ///< 次ホップMACアドレス（小文字コロン区切り、例: "cc:7b:5c:9a:f3:ac"）
  bool valid;            ///< エントリが有効かどうか
};

/// @brief FIB初期エントリの最大数
constexpr size_t MAX_FIB_INIT_ENTRIES = 10;

struct SystemConfig {
  size_t maxPitTableSize = MAX_PIT_TABLE_SIZE;
  size_t maxCsTableSize = MAX_CS_TABLE_SIZE;
  uint32_t cacheEntryTtlUs = 100000000; // 100秒
  size_t maxFibTableSize = MAX_FIB_TABLE_SIZE;
  int maxVirtualDepth = 5;
  int hopCountThreshold = 10;

  // セキュリティ設定
  uint8_t pmk[ESP_NOW_PMK_LEN] = {0};  // Primary Master Key
  uint8_t lmk[ESP_NOW_LMK_LEN] = {0};  // グローバルLocal Master Key（ピア固有LMK未設定時に使用）
  bool encryptionEnabled = false;

  // ピア固有LMK設定
  PeerLMKConfig peerLmkEntries[MAX_PEER_LMK_ENTRIES];
  size_t peerLmkCount = 0;

  // FIB初期エントリ（テスト用ブランチで多段経路を事前設定するために使用）
  FibInitEntry fibInitEntries[MAX_FIB_INIT_ENTRIES];
  size_t fibInitCount = 0;
};

extern SystemConfig systemConfig;

bool loadSystemConfig(const char* path = "/config.json");
