#include "ESP-NOWController.hpp"
#include "BuildProfile.hpp"
#include "config/Config.hpp"
#include "message/SignalCode.hpp"

#include <string>
#include <sstream>
#include <iomanip>
#include <cstring> // strncpy用

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
        outputData = useCaseInteractor.handleInterestReceive(inputData);
    }
    else if (code == SignalCode::DATA)
    {
        outputData = useCaseInteractor.handleDataReceive(inputData);
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

    useCaseInteractor.handleSensorDataReceive(inputData);
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
    useCaseInteractor.initFIBEntry(contentName, nextHopMac);
}

void ESP_NOWController::printFIB() const
{
    useCaseInteractor.printFIB();
}
