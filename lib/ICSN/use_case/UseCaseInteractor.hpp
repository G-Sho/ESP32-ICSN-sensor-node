#pragma once

#include "../interface/InputBoundary.hpp"
#include "../interface/ManagementBoundary.hpp"
#include "../interface/data_access/IForwardingInformationBase.hpp"
#include "../interface/data_access/IPendingInterestTable.hpp"
#include "../interface/data_access/IContentStore.hpp"
#include "../data_structure/InputData.hpp"
#include "../data_structure/OutputData.hpp"

class UseCaseInteractor : public IInputBoundary, public IManagementBoundary
{
private:
    IForwardingInformationBase &fibRepository;
    IPendingInterestTable &pitRepository;
    IContentStore &csRepository;

public:
    UseCaseInteractor(IForwardingInformationBase &fibRepository,
                      IPendingInterestTable &pitRepository,
                      IContentStore &csRepository);

    virtual ~UseCaseInteractor() = default;

    // Interestパケット受信時の処理
    virtual OutputData handleInterestReceive(const InputData &inputData) override;

    // Dataパケット受信時の処理
    virtual OutputData handleDataReceive(const InputData &inputData) override;

    // センサーデータ受信時の処理
    virtual void handleSensorDataReceive(const InputData &inputData) override;

    // FIB初期エントリを投入する（起動時にテスト用ルーティングを設定するために使用）
    void initFIBEntry(const std::string &contentName, const std::string &nextHopMac) override;

    // FIBの内容をシリアルに出力する
    void printFIB() const override;

    // Content Store をクリアする
    void clearCSCache() override;

    // PIT をクリアする
    void clearPITCache() override;

#ifdef UNIT_TEST
    void mockAddToCS(const std::string &name, const std::string &content);
    void mockAddToFIB(const std::string &name, uint32_t nextHop);
    void mockAddToPIT(const std::string &name, uint32_t fromNode);
#endif
};
