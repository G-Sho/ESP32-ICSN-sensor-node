#include "ESP-NOWController.hpp"
#include "message/SignalCode.hpp"

#include <string>
#include <sstream>
#include <iomanip>

// Helper: Convert addresses to std::string for logging
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

    Serial.printf("Received message from %s to %s\n", txAddrStr.c_str(), rxAddrStr.c_str());

    SignalCode code = fromString(data.signalCode.c_str());

    InputData inputData(
        txAddrStr,
        {rxAddrStr},
        std::string(data.signalCode.c_str()),
        static_cast<int>(data.hopCount),
        std::string(data.contentName.c_str()),
        std::string(data.content.c_str()),
        0);

    if (code == SignalCode::INTEREST)
    {
        OutputData outputData = useCaseInteractor.handleInterestReceive(inputData);
        ESP_NOWControlData result;

        // 最大20個までコピー
        int index = 0;
        for (const auto &addrStr : outputData.destId)
        {
            if (index >= 20)
                break; // 配列の上限
            result.txAddress[index] = macStringToArray(addrStr);
            index++;
        }

        result.signalCode = String(outputData.signalCode.c_str());
        result.hopCount = outputData.hopCount;
        result.contentName = String(outputData.contentName.c_str());
        result.content = String(outputData.content.c_str());

        return result;
    }
    else if (code == SignalCode::DATA)
    {
        OutputData outputData = useCaseInteractor.handleDataReceive(inputData);
        ESP_NOWControlData result;

        // 最大20個までコピー
        int index = 0;
        for (const auto &addrStr : outputData.destId)
        {
            if (index >= 20)
                break; // 配列の上限
            result.txAddress[index] = macStringToArray(addrStr);
            index++;
        }

        result.signalCode = String(outputData.signalCode.c_str());
        result.hopCount = outputData.hopCount;
        result.contentName = String(outputData.contentName.c_str());
        result.content = String(outputData.content.c_str());

        return result;
    }
    else
    {
        Serial.println("Unknown signal code received");
        return ESP_NOWControlData{};
    }
}

void ESP_NOWController::receiveSensorData(const ESP_NOWControlData &data)
{
    // Log the received sensor data
    Serial.printf("Received sensor data: %s\n", data.content.c_str());

    // Process the sensor data
    InputData inputData(
        std::string("0"), // senderId: 仮の "0" という文字列
        {},               // destId: 空の std::set<std::string>
        std::string(data.signalCode.c_str()),
        static_cast<int>(data.hopCount),
        std::string(data.contentName.c_str()),
        std::string(data.content.c_str()),
        millis());
    useCaseInteractor.handleSensorDataReceive(inputData);
}