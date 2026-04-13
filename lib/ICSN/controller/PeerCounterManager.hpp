#pragma once

#include <Arduino.h>
#include <cstdint>
#include <cstring>

/// @brief ピアごとのカウンタ情報
struct PeerCounter {
  uint8_t peer_mac[6];
  uint32_t tx_counter;
  uint32_t rx_counter;
  bool active;
};

/// @brief ピアごとの送受信カウンタを管理するクラス（リプレイ攻撃対策）
class PeerCounterManager {
private:
  static constexpr size_t MAX_PEERS = 20;
  PeerCounter peers[MAX_PEERS];

  int findPeerIndex(const uint8_t mac[6]) const {
    for (size_t i = 0; i < MAX_PEERS; i++) {
      if (peers[i].active && memcmp(peers[i].peer_mac, mac, 6) == 0) {
        return static_cast<int>(i);
      }
    }
    return -1;
  }

  int findOrCreatePeerIndex(const uint8_t mac[6]) {
    int idx = findPeerIndex(mac);
    if (idx >= 0) return idx;

    // 空きスロットを探す
    for (size_t i = 0; i < MAX_PEERS; i++) {
      if (!peers[i].active) {
        memcpy(peers[i].peer_mac, mac, 6);
        peers[i].tx_counter = 0;
        peers[i].rx_counter = 0;
        peers[i].active = true;
        return static_cast<int>(i);
      }
    }
    return -1; // スロット不足
  }

public:
  PeerCounterManager() {
    memset(peers, 0, sizeof(peers));
  }

  /// @brief 送信カウンタをインクリメントし、現在の値を返す
  /// @param mac 送信先MACアドレス
  /// @return インクリメント後のカウンタ値。スロット不足時は0
  uint32_t incrementTxCounter(const uint8_t mac[6]) {
    int idx = findOrCreatePeerIndex(mac);
    if (idx < 0) return 0;
    peers[idx].tx_counter++;
    return peers[idx].tx_counter;
  }

  /// @brief ユニキャスト受信時のカウンタ検証
  /// @param mac 送信元MACアドレス
  /// @param received_counter メッセージに含まれるカウンタ値
  /// @return true: 検証成功, false: リプレイ攻撃検知
  bool validateRxCounter(const uint8_t mac[6], uint32_t received_counter) {
    int idx = findOrCreatePeerIndex(mac);
    if (idx < 0) return false;

    uint32_t expected = peers[idx].rx_counter + 1;
    if (received_counter == expected) {
      peers[idx].rx_counter = received_counter;
      return true;
    }
    return false;
  }
};
