#pragma once

#include <cstdint>
#include <string>

#include "../interface/InputBoundary.hpp"
#include "../interface/ForwardingStateBoundary.hpp"
#include "../interface/data_access/IForwardingInformationBase.hpp"
#include "../interface/data_access/IPendingInterestTable.hpp"
#include "../interface/data_access/IContentStore.hpp"
#include "../interface/data_access/IRIB.hpp"
#include "../data_structure/InputData.hpp"
#include "../data_structure/OutputData.hpp"

class UseCaseInteractor : public IInputBoundary, public IForwardingStateBoundary
{
private:
    IForwardingInformationBase &fibRepository;
    IPendingInterestTable &pitRepository;
    IContentStore &csRepository;
    IRIB &ribRepository;

public:
    UseCaseInteractor(IForwardingInformationBase &fibRepository,
                      IPendingInterestTable &pitRepository,
                      IContentStore &csRepository,
                      IRIB &ribRepository);

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
};
