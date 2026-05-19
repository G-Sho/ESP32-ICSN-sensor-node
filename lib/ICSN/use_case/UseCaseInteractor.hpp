#pragma once

#include "../interface/InputBoundary.hpp"
#include "../data_structure/InputData.hpp"
#include "../data_structure/OutputData.hpp"
// #include "data_access/TwoStageLookupFIBRepository.hpp"
#include "../infrastructure/data_access/LRUForwardingInformationBase.hpp"
#include "../infrastructure/data_access/LRUPendingInterestTable.hpp"
#include "../infrastructure/data_access/LRUContentStore.hpp"
// #include "data_access/pCASTINGCSRepository.hpp"

class UseCaseInteractor : public IInputBoundary
{
private:
    // FIB, PIT, CSのリポジトリ
    // TwoStageLookupFIBRepository fibRepository;
    LRUForwardingInformationBase fibRepository;
    LRUPendingInterestTable pitRepository;
    LRUContentStore csRepository;
    // pCASTINGCSRepository csRepository;

public:
    virtual ~UseCaseInteractor() = default;

    // Interestパケット受信時の処理
    virtual OutputData handleInterestReceive(const InputData &inputData) override;

    // Dataパケット受信時の処理
    virtual OutputData handleDataReceive(const InputData &inputData) override;

    // センサーデータ受信時の処理
    virtual void handleSensorDataReceive(const InputData &inputData) override;

    // FIB初期エントリを投入する（起動時にテスト用ルーティングを設定するために使用）
    void initFIBEntry(const std::string& contentName, const std::string& nextHopMac);

    // FIBの内容をシリアルに出力する
    void printFIB() const;

    // Content Store をクリアする
    void clearCSCache();

    // PIT をクリアする
    void clearPITCache();

#ifdef UNIT_TEST
    void mockAddToCS(const std::string &name, const std::string &content);
    void mockAddToFIB(const std::string &name, uint32_t nextHop);
    void mockAddToPIT(const std::string &name, uint32_t fromNode);
#endif
};
