#pragma once

#include "UseCaseInteractor.hpp"
#include "ESP-NOWControlData.hpp"
#include "InputData.hpp"
#include "OutputData.hpp"

/// @brief ESP-NOWController class
class ESP_NOWController
{
private:
  UseCaseInteractor useCaseInteractor;

public:
    ESP_NOWControlData receiveMessage(const uint8_t rxAddress[6], const uint8_t txAddress[6], const ESP_NOWControlData &data);
    void receiveSensorData(const ESP_NOWControlData &data);
};