#pragma once

#include "UseCaseInteractor.hpp"
#include "ESP-NOWControlData.hpp"
#include "PeerCounterManager.hpp"
#include "InputData.hpp"
#include "OutputData.hpp"

#include <cstddef>

/// @brief ESP-NOWControllerクラス
class ESP_NOWController
{
private:
    static constexpr size_t PMK_LENGTH = 16;

    UseCaseInteractor useCaseInteractor;
    PeerCounterManager peerCounterManager;
    bool encryptionEnabled = false;
    uint8_t pmk[PMK_LENGTH] = {0};

public:
    bool loadAndApplyConfig(const char *configPath = "/config.json");
    bool copyPMK(uint8_t *outPmk, size_t outLen) const;

    ESP_NOWControlData receiveMessage(const uint8_t rxAddress[6], const uint8_t txAddress[6], const ESP_NOWControlData &data);
    void receiveSensorData(const ESP_NOWControlData &data);

    // セキュリティ設定
    void setGlobalLMK(const uint8_t lmk[PEER_LMK_LEN]);
    bool setPeerLMK(const uint8_t mac[6], const uint8_t lmk[PEER_LMK_LEN]);

    // 送信先ごとのパケット生成（applySecurity=true の場合は counter/HMAC を付与）
    bool buildPacketForAddress(const uint8_t txAddress[6],
                               const ESP_NOWControlData &data,
                               bool applySecurity,
                               CommunicationData &outPacket);

    // 受信ユニキャストパケットのHMAC/counter検証
    bool verifyIncomingPacket(const uint8_t mac[6], const CommunicationData &packet);

    // Peer counter状態を出力
    void printCounters() const;

    // FIB初期エントリを投入する（起動時にテスト用ルーティングを設定するために使用）
    void initFIBEntry(const std::string& contentName, const std::string& nextHopMac);

    // FIBの内容をシリアルに出力する
    void printFIB() const;

    // Content Store をクリアする
    void clearCSCache() {
        useCaseInteractor.clearCSCache();
    }

    // PIT をクリアする
    void clearPITCache() {
        useCaseInteractor.clearPITCache();
    }
};
