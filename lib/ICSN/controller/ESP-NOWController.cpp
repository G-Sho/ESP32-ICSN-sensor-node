#include "ESP-NOWController.hpp"
#include "BuildProfile.hpp"
#include "config/Config.hpp"
#include "message/SignalCode.hpp"

#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>

#include <string>
#include <sstream>
#include <iomanip>
#include <cstring> // strncpy用
#include <algorithm>

namespace {
constexpr uint8_t BROADCAST_ADDRESS[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
}

ESP_NOWController::ESP_NOWController(IInputBoundary &inputBoundary,
                                     IManagementBoundary &managementBoundary)
    : inputBoundary(inputBoundary),
      managementBoundary(managementBoundary)
{
}

bool ESP_NOWController::loadAndApplyConfig(const char *configPath)
{
    if (!loadSystemConfig(configPath))
    {
        return false;
    }

    encryptionEnabled = systemConfig.encryptionEnabled;
    memset(pmk, 0, sizeof(pmk));

    if (encryptionEnabled)
    {
        memcpy(pmk, systemConfig.pmk, sizeof(pmk));
        setGlobalLMK(systemConfig.lmk);
        LOG_INFO("[SECURITY] Global LMK configured for HMAC");
    }

    for (size_t i = 0; i < systemConfig.peerLmkCount; i++)
    {
        const PeerLMKConfig &entry = systemConfig.peerLmkEntries[i];
        if (entry.valid)
        {
            setPeerLMK(entry.mac, entry.lmk);
        }
    }

    for (size_t i = 0; i < systemConfig.fibInitCount; i++)
    {
        const FibInitEntry &entry = systemConfig.fibInitEntries[i];
        if (entry.valid)
        {
            initFIBEntry(std::string(entry.contentName), std::string(entry.nextHopMac));
            LOG_INFOF("[FIB] Initial entry: %s -> %s\n", entry.contentName, entry.nextHopMac);
        }
    }

    return true;
}

bool ESP_NOWController::copyPMK(uint8_t *outPmk, size_t outLen) const
{
    if (outPmk == nullptr || outLen < sizeof(pmk) || !encryptionEnabled)
    {
        return false;
    }

    memcpy(outPmk, pmk, sizeof(pmk));
    return true;
}

bool ESP_NOWController::initializeCommunication(const char *configPath,
                                                uint8_t myMac[6],
                                                esp_now_recv_cb_t recvCb,
                                                esp_now_send_cb_t sendCb,
                                                uint8_t channel)
{
    if (myMac == nullptr || recvCb == nullptr || sendCb == nullptr)
    {
        return false;
    }

    if (!loadAndApplyConfig(configPath))
    {
        return false;
    }

    WiFi.mode(WIFI_STA);
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);

    if (esp_now_init() != ESP_OK)
    {
        LOG_WARN("ESP-NOW initialization failed");
        return false;
    }

    uint8_t localPmk[PMK_LENGTH] = {0};
    if (copyPMK(localPmk, sizeof(localPmk)))
    {
        if (esp_now_set_pmk(localPmk) != ESP_OK)
        {
            LOG_WARN("Failed to set PMK");
            return false;
        }
        LOG_INFO("ESP-NOW encryption enabled (PMK/LMK configured)");
    }

    esp_wifi_get_mac(WIFI_IF_STA, myMac);
    esp_now_register_send_cb(sendCb);
    esp_now_register_recv_cb(recvCb);
    registerBroadcastPeer();

    LOG_INFO("ESP-NOW initialized successfully");
    return true;
}

bool ESP_NOWController::isBroadcastAddress(const std::array<uint8_t, 6> &addr)
{
    return std::all_of(addr.begin(), addr.end(), [](uint8_t b) { return b == 0xFF; });
}

void ESP_NOWController::registerPeerIfNeeded(const uint8_t mac[6])
{
    if (mac == nullptr || esp_now_is_peer_exist(mac))
    {
        return;
    }

    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, mac, 6);
    peer.channel = 0;
    peer.ifidx = WIFI_IF_STA;
    peer.encrypt = false;

    if (esp_now_add_peer(&peer) != ESP_OK)
    {
        LOG_WARN("[PEER] Failed to register peer");
    }
}

void ESP_NOWController::registerBroadcastPeer()
{
    registerPeerIfNeeded(BROADCAST_ADDRESS);
}

bool ESP_NOWController::sendPacketToAddresses(const ESP_NOWControlData &data)
{
    bool sentAny = false;

    for (const auto &addr : data.txAddress)
    {
        if (std::all_of(addr.begin(), addr.end(), [](uint8_t b) { return b == 0; }))
        {
            continue;
        }

        bool isBcast = isBroadcastAddress(addr);
        CommunicationData packet = {};
        if (!buildPacketForAddress(addr.data(), data, !isBcast, packet))
        {
            continue;
        }

        registerPeerIfNeeded(addr.data());

        esp_err_t err = esp_now_send(addr.data(), reinterpret_cast<const uint8_t *>(&packet), sizeof(packet));
        if (err != ESP_OK)
        {
            LOG_WARNF("[TX] esp_now_send error: %d\n", err);
            continue;
        }

        sentAny = true;
    }

    return sentAny;
}

bool ESP_NOWController::sendSensorData(const char *contentName, const char *content, uint8_t hopCount)
{
    if (contentName == nullptr || content == nullptr)
    {
        return false;
    }

    ESP_NOWControlData sensorData = {};
    sensorData.hopCount = hopCount;
    strncpy(sensorData.signalCode, "DATA", MAX_SIGNAL_CODE_LENGTH - 1);
    sensorData.signalCode[MAX_SIGNAL_CODE_LENGTH - 1] = '\0';
    strncpy(sensorData.contentName, contentName, MAX_CONTENT_NAME_LENGTH - 1);
    sensorData.contentName[MAX_CONTENT_NAME_LENGTH - 1] = '\0';
    strncpy(sensorData.content, content, MAX_CONTENT_LENGTH - 1);
    sensorData.content[MAX_CONTENT_LENGTH - 1] = '\0';

    receiveSensorData(sensorData);
    return true;
}

bool ESP_NOWController::sendInterest(const char *contentName, const uint8_t *targetMac, uint8_t hopCount)
{
    if (contentName == nullptr)
    {
        return false;
    }

    ESP_NOWControlData interest = {};
    if (targetMac == nullptr)
    {
        std::copy(BROADCAST_ADDRESS, BROADCAST_ADDRESS + 6, interest.txAddress[0].begin());
    }
    else
    {
        std::copy(targetMac, targetMac + 6, interest.txAddress[0].begin());
    }

    strncpy(interest.signalCode, "INTEREST", MAX_SIGNAL_CODE_LENGTH - 1);
    interest.signalCode[MAX_SIGNAL_CODE_LENGTH - 1] = '\0';
    interest.hopCount = hopCount;
    strncpy(interest.contentName, contentName, MAX_CONTENT_NAME_LENGTH - 1);
    interest.contentName[MAX_CONTENT_NAME_LENGTH - 1] = '\0';
    strncpy(interest.content, "N/A", MAX_CONTENT_LENGTH - 1);
    interest.content[MAX_CONTENT_LENGTH - 1] = '\0';

    return sendPacketToAddresses(interest);
}

bool ESP_NOWController::processReceivedPacket(const uint8_t myMac[6],
                                              const uint8_t senderMac[6],
                                              const uint8_t *data,
                                              int len,
                                              ReceiveProcessResult *result)
{
    ReceiveProcessResult localResult;
    ReceiveProcessResult &out = (result == nullptr) ? localResult : *result;
    out = {};

    if (myMac == nullptr || senderMac == nullptr || data == nullptr || len != static_cast<int>(sizeof(CommunicationData)))
    {
        return false;
    }

    registerPeerIfNeeded(senderMac);

    CommunicationData receivedPacket = {};
    memcpy(&receivedPacket, data, sizeof(receivedPacket));
    out.validPacket = true;

    out.isInterest = (strncmp(receivedPacket.signalCode, "INTEREST", MAX_SIGNAL_CODE_LENGTH) == 0);
    out.isData = (strncmp(receivedPacket.signalCode, "DATA", MAX_SIGNAL_CODE_LENGTH) == 0);

#if ICSN_PERF_ENABLED
    if (out.isInterest)
    {
        interestTiming.recordInterestRx();
    }
#endif

    std::array<uint8_t, 6> macArray = {};
    std::copy(senderMac, senderMac + 6, macArray.begin());
    bool isBroadcast = isBroadcastAddress(macArray);

    out.securityCheckRequired = !isBroadcast;
    if (out.securityCheckRequired)
    {
#if ICSN_PERF_ENABLED
        if (out.isInterest)
        {
            interestTiming.recordSecurityCheckStart();
        }
#endif
        out.securityCheckVerified = verifyIncomingPacket(senderMac, receivedPacket);
        if (!out.securityCheckVerified)
        {
            return false;
        }

#if ICSN_PERF_ENABLED
        if (out.isInterest)
        {
            interestTiming.recordSecurityCheckEnd();
        }
#endif
    }

    ESP_NOWControlData inputData = {};
    strncpy(inputData.signalCode, receivedPacket.signalCode, MAX_SIGNAL_CODE_LENGTH - 1);
    inputData.signalCode[MAX_SIGNAL_CODE_LENGTH - 1] = '\0';
    inputData.hopCount = receivedPacket.hopCount;
    strncpy(inputData.contentName, receivedPacket.contentName, MAX_CONTENT_NAME_LENGTH - 1);
    inputData.contentName[MAX_CONTENT_NAME_LENGTH - 1] = '\0';
    strncpy(inputData.content, receivedPacket.content, MAX_CONTENT_LENGTH - 1);
    inputData.content[MAX_CONTENT_LENGTH - 1] = '\0';
    std::copy(senderMac, senderMac + 6, inputData.txAddress[0].begin());

    ESP_NOWControlData outputData = receiveMessage(myMac, senderMac, inputData);

#if ICSN_PERF_ENABLED
    if (out.isInterest)
    {
        interestTiming.recordFibLookup();
    }
#endif

    out.forwarded = sendPacketToAddresses(outputData);

#if ICSN_PERF_ENABLED
    if (out.isInterest && out.forwarded)
    {
        interestTiming.recordForwardTx();
    }
#endif

    return true;
}

// ヘルパー: アドレスをログ用のstd::stringに変換
static std::string addressToString(const uint8_t *address)
{
    std::ostringstream oss;
    for (int i = 0; i < 6; ++i)
    {
        if (i > 0)
            oss << ":";
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(address[i]);
    }
    return oss.str();
}

std::array<uint8_t, 6> macStringToArray(const std::string &macStr)
{
    std::array<uint8_t, 6> mac{};
    unsigned int values[6];

    if (sscanf(macStr.c_str(), "%x:%x:%x:%x:%x:%x",
               &values[0], &values[1], &values[2],
               &values[3], &values[4], &values[5]) == 6)
    {
        for (int i = 0; i < 6; ++i)
        {
            mac[i] = static_cast<uint8_t>(values[i]);
        }
    }
    return mac;
}

ESP_NOWControlData ESP_NOWController::receiveMessage(const uint8_t rxAddress[6], const uint8_t txAddress[6], const ESP_NOWControlData &data)
{
    std::string rxAddrStr = addressToString(rxAddress);
    std::string txAddrStr = addressToString(txAddress);

    LOG_DEBUGF("Received message from %s to %s\n", txAddrStr.c_str(), rxAddrStr.c_str());

    SignalCode code = fromString(data.signalCode);

    InputData inputData(
        txAddrStr,
        {rxAddrStr},
        std::string(data.signalCode),
        static_cast<int>(data.hopCount),
        std::string(data.contentName),
        std::string(data.content));

    OutputData outputData;

    if (code == SignalCode::INTEREST)
    {
        outputData = inputBoundary.handleInterestReceive(inputData);
    }
    else if (code == SignalCode::DATA)
    {
        outputData = inputBoundary.handleDataReceive(inputData);
    }
    else
    {
        LOG_WARNF("Unknown signal code received\n");
        return ESP_NOWControlData{};
    }

    ESP_NOWControlData result = {};

    // 最大MAX_TX_ADDRESSES個までMACアドレスをコピー
    int index = 0;
    for (const auto &addrStr : outputData.destId)
    {
        if (index >= MAX_TX_ADDRESSES)
            break;
        result.txAddress[index] = macStringToArray(addrStr);
        index++;
    }

    // char配列への安全なコピー
    strncpy(result.signalCode, outputData.signalCode.c_str(), MAX_SIGNAL_CODE_LENGTH - 1);
    result.signalCode[MAX_SIGNAL_CODE_LENGTH - 1] = '\0';

    result.hopCount = outputData.hopCount;

    strncpy(result.contentName, outputData.contentName.c_str(), MAX_CONTENT_NAME_LENGTH - 1);
    result.contentName[MAX_CONTENT_NAME_LENGTH - 1] = '\0';

    strncpy(result.content, outputData.content.c_str(), MAX_CONTENT_LENGTH - 1);
    result.content[MAX_CONTENT_LENGTH - 1] = '\0';

    return result;
}

void ESP_NOWController::receiveSensorData(const ESP_NOWControlData &data)
{
    LOG_INFOF("Received sensor data: %s\n", data.content);

    InputData inputData(
        std::string("N/A"), // senderId: 仮
        {},                 // destId: 空
        std::string(data.signalCode),
        0, // hopCount: 空
        std::string(data.contentName),
        std::string(data.content));

    inputBoundary.handleSensorDataReceive(inputData);
}

void ESP_NOWController::setGlobalLMK(const uint8_t lmk[PEER_LMK_LEN])
{
    peerCounterManager.setGlobalLMK(lmk);
}

bool ESP_NOWController::setPeerLMK(const uint8_t mac[6], const uint8_t lmk[PEER_LMK_LEN])
{
    return peerCounterManager.setPeerLMK(mac, lmk);
}

bool ESP_NOWController::buildPacketForAddress(const uint8_t txAddress[6],
                                              const ESP_NOWControlData &data,
                                              bool applySecurity,
                                              CommunicationData &outPacket)
{
    memset(&outPacket, 0, sizeof(CommunicationData));

    strncpy(outPacket.signalCode, data.signalCode, MAX_SIGNAL_CODE_LENGTH - 1);
    outPacket.signalCode[MAX_SIGNAL_CODE_LENGTH - 1] = '\0';
    outPacket.hopCount = data.hopCount;
    strncpy(outPacket.contentName, data.contentName, MAX_CONTENT_NAME_LENGTH - 1);
    outPacket.contentName[MAX_CONTENT_NAME_LENGTH - 1] = '\0';
    strncpy(outPacket.content, data.content, MAX_CONTENT_LENGTH - 1);
    outPacket.content[MAX_CONTENT_LENGTH - 1] = '\0';

    if (!applySecurity)
    {
        outPacket.counter = 0;
        memset(outPacket.hmac, 0, sizeof(outPacket.hmac));
        return true;
    }

    bool counterSuccess = false;
    outPacket.counter = peerCounterManager.incrementTxCounter(txAddress, counterSuccess);
    if (!counterSuccess)
    {
        return false;
    }

    memset(outPacket.hmac, 0, sizeof(outPacket.hmac));
    if (!peerCounterManager.computeHMAC(txAddress,
                                        reinterpret_cast<const uint8_t *>(&outPacket),
                                        COMM_DATA_HMAC_DATA_LEN,
                                        outPacket.hmac))
    {
        LOG_WARN("[SECURITY] HMAC computation failed");
        return false;
    }

    return true;
}

bool ESP_NOWController::verifyIncomingPacket(const uint8_t mac[6], const CommunicationData &packet)
{
    if (!peerCounterManager.verifyHMAC(mac,
                                       reinterpret_cast<const uint8_t *>(&packet),
                                       COMM_DATA_HMAC_DATA_LEN,
                                       packet.hmac))
    {
        LOG_WARN("[SECURITY] HMAC verification FAILED");
        return false;
    }

    if (!peerCounterManager.validateRxCounter(mac, packet.counter))
    {
        LOG_WARNF("[SECURITY] Replay attack detected! counter=%lu\n",
                  (unsigned long)packet.counter);
        return false;
    }

    return true;
}

void ESP_NOWController::printCounters() const
{
    peerCounterManager.printCounters();
}

void ESP_NOWController::initFIBEntry(const std::string& contentName, const std::string& nextHopMac)
{
    managementBoundary.initFIBEntry(contentName, nextHopMac);
}

void ESP_NOWController::printFIB() const
{
    managementBoundary.printFIB();
}

void ESP_NOWController::clearCSCache()
{
    managementBoundary.clearCSCache();
}

void ESP_NOWController::clearPITCache()
{
    managementBoundary.clearPITCache();
}

void ESP_NOWController::dumpPerformanceData() const
{
#if !ICSN_PERF_ENABLED
    CLI_PRINTLN("{\"error\": \"perf_build_required\"}");
    return;
#else
    uint16_t cnt = interestTiming.getCount();
    CLI_PRINTLN("{");
    CLI_PRINTLN("  \"measurements\": [");
    for (uint16_t i = 0; i < cnt; i++)
    {
        const InterestPacketTimingEntry &m = interestTiming.getEntry(i);
        const char *separator = (i < cnt - 1) ? "," : "";
        uint32_t security_check_us = m.security_check_end_us - m.security_check_start_us;
        uint32_t fib_us = (m.security_check_end_us > 0)
                      ? m.fib_lookup_us - m.security_check_end_us
                              : m.fib_lookup_us - m.interest_rx_us;
        uint32_t total_us = m.forward_tx_us - m.interest_rx_us;
        CLI_PRINTF("    {\"i\": %u, \"security_check_us\": %lu, \"fib_us\": %lu, \"total_us\": %lu}%s\n",
                   static_cast<unsigned>(i),
               static_cast<unsigned long>(security_check_us),
                   static_cast<unsigned long>(fib_us),
                   static_cast<unsigned long>(total_us),
                   separator);
    }
    CLI_PRINTLN("  ]");
    CLI_PRINTLN("}");
#endif
}

void ESP_NOWController::resetPerformanceData()
{
#if !ICSN_PERF_ENABLED
    CLI_PRINTLN("{\"error\": \"perf_build_required\"}");
#else
    interestTiming.reset();
    CLI_PRINTLN("{\"status\": \"perf_reset\"}");
#endif
}

void ESP_NOWController::printPerformanceCount() const
{
#if !ICSN_PERF_ENABLED
    CLI_PRINTLN("{\"error\": \"perf_build_required\"}");
#else
    CLI_PRINTF("{\"count\": %u}\n", static_cast<unsigned>(interestTiming.getCount()));
#endif
}
