#pragma once

#include "../interface/InputBoundary.hpp"
#include "../interface/ManagementBoundary.hpp"
#include "ESP-NOWControlData.hpp"
#include "PeerCounterManager.hpp"
#include "InputData.hpp"
#include "OutputData.hpp"
#include "performance/InterestPacketTimingBuffer.hpp"

#include <esp_now.h>

#include <cstddef>
#include <array>

/// @brief ESP_NOWControllerクラス（ファイル名: ESP-NOWController.hpp）
class ESP_NOWController
{
private:
    static constexpr size_t PMK_LENGTH = 16;

    IInputBoundary &inputBoundary;
    IManagementBoundary &managementBoundary;
    PeerCounterManager peerCounterManager;
    InterestPacketTimingBuffer interestTiming;
    bool encryptionEnabled = false;
    uint8_t pmk[PMK_LENGTH] = {0};

    ESP_NOWControlData receiveMessage(const uint8_t rxAddress[6], const uint8_t txAddress[6], const ESP_NOWControlData &data);
    void receiveSensorData(const ESP_NOWControlData &data);
    bool buildPacketForAddress(const uint8_t txAddress[6],
                               const ESP_NOWControlData &data,
                               bool applySecurity,
                               CommunicationData &outPacket);
    bool verifyIncomingPacket(const uint8_t mac[6], const CommunicationData &packet);
    bool sendPacketToAddresses(const ESP_NOWControlData &data);
    static bool isBroadcastAddress(const std::array<uint8_t, 6> &addr);

public:
    ESP_NOWController(IInputBoundary &inputBoundary, IManagementBoundary &managementBoundary);

    struct ReceiveProcessResult
    {
        bool validPacket = false;
        bool isInterest = false;
        bool isData = false;
        bool securityCheckRequired = false;
        bool securityCheckVerified = false;
        bool forwarded = false;
    };

    bool loadAndApplyConfig(const char *configPath = "/config.json");
    bool initializeCommunication(const char *configPath,
                                 uint8_t myMac[6],
                                 esp_now_recv_cb_t recvCb,
                                 esp_now_send_cb_t sendCb,
                                 uint8_t channel = 1);
    bool copyPMK(uint8_t *outPmk, size_t outLen) const;

    void registerPeerIfNeeded(const uint8_t mac[6]);
    void registerBroadcastPeer();
    bool sendSensorData(const char *contentName, const char *content, uint8_t hopCount = 1);
    bool sendInterest(const char *contentName, const uint8_t *targetMac = nullptr, uint8_t hopCount = 1);
    bool processReceivedPacket(const uint8_t myMac[6],
                               const uint8_t senderMac[6],
                               const uint8_t *data,
                               int len,
                               ReceiveProcessResult *result = nullptr);

    // セキュリティ設定
    void setGlobalLMK(const uint8_t lmk[PEER_LMK_LEN]);
    bool setPeerLMK(const uint8_t mac[6], const uint8_t lmk[PEER_LMK_LEN]);

    // Peer counter状態を出力
    void printCounters() const;

    // FIB初期エントリを投入する（起動時にテスト用ルーティングを設定するために使用）
    void initFIBEntry(const std::string& contentName, const std::string& nextHopMac);

    // FIBの内容をシリアルに出力する
    void printFIB() const;

    // stage-based performance results (controller-owned)
    void dumpPerformanceData() const;
    void resetPerformanceData();
    void printPerformanceCount() const;

    // Content Store をクリアする
    void clearCSCache();

    // PIT をクリアする
    void clearPITCache();
};
