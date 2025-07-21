#pragma once

#include "interface/InputBoundary.hpp"
#include "InputData.hpp"
#include "OutputData.hpp"
#include "data_access/TwoStageLookupFIBRepository.hpp"
#include "data_access/LRUPITRepository.hpp"
#include "data_access/pCASTINGCSRepository.hpp"

class UseCaseInteractor : public IInputBoundary
{
private:
    // FIB, PIT, CSのリポジトリ
    TwoStageLookupFIBRepository fibRepository;
    LRUPITRepository pitRepository;
    pCASTINGCSRepository csRepository;

public:
    virtual ~UseCaseInteractor() = default;

    // メッシュの設定
    void setMesh(painlessMesh *meshPtr)
    {
        if (meshPtr == nullptr)
        {
            Serial.println("Error: Mesh pointer is null.");
            return;
        }
        csRepository.setMesh(meshPtr);
    };

    // Interestパケット受信時の処理
    virtual OutputData handleInterestReceive(const InputData &inputData) override;

    // Dataパケット受信時の処理
    virtual OutputData handleDataReceive(const InputData &inputData) override;

    // センサーデータ受信時の処理
    virtual void handleSensorDataReceive(const InputData &inputData) override;

#ifdef UNIT_TEST
    void mockAddToCS(const std::string &name, const std::string &content);
    void mockAddToFIB(const std::string &name, uint32_t nextHop);
    void mockAddToPIT(const std::string &name, uint32_t fromNode);
#endif
};