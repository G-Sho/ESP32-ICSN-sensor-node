#include "ESP-NOWController.hpp"
#include "message/SignalCode.hpp"

#include <string>
#include <sstream>
#include <iomanip>
#include <cstring> // strncpy用

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

    // Serial.printf("Received message from %s to %s\n", txAddrStr.c_str(), rxAddrStr.c_str());

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
        Serial.println("Unknown signal code received");
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
    // Serial.printf("Received sensor data: %s\n", data.content);

    InputData inputData(
        std::string("N/A"), // senderId: 仮
        {},                 // destId: 空
        std::string(data.signalCode),
        0, // hopCount: 空
        std::string(data.contentName),
        std::string(data.content));

    useCaseInteractor.handleSensorDataReceive(inputData);
}

void ESP_NOWController::initFIBEntry(const std::string& contentName, const std::string& nextHopMac)
{
    useCaseInteractor.initFIBEntry(contentName, nextHopMac);
}

void ESP_NOWController::printFIB() const
{
    useCaseInteractor.printFIB();
}
