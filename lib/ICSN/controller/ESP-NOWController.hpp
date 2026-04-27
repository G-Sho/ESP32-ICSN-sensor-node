#pragma once

#include "UseCaseInteractor.hpp"
#include "ESP-NOWControlData.hpp"
#include "InputData.hpp"
#include "OutputData.hpp"

/// @brief ESP-NOWControllerクラス
class ESP_NOWController
{
private:
  UseCaseInteractor useCaseInteractor;

public:
    ESP_NOWControlData receiveMessage(const uint8_t rxAddress[6], const uint8_t txAddress[6], const ESP_NOWControlData &data);
    void receiveSensorData(const ESP_NOWControlData &data);

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