#pragma once

#include "InputData.hpp"
#include "data_structure/OutputData.hpp"

class IInputBoundary {
public:
    // Interestパケット受信時の処理
    virtual OutputData handleInterestReceive(const InputData& inputData) = 0;

    // Dataパケット受信時の処理
    virtual OutputData handleDataReceive(const InputData& inputData) = 0;

    // センサーデータ受信時の処理
    virtual void handleSensorDataReceive(const InputData& inputData) = 0;

    virtual ~IInputBoundary() = default;
};