#include "UseCaseInteractor.hpp"
#include "message/SenderId.hpp"
#include "message/DestinationId.hpp"
#include "message/SignalCode.hpp"
#include "message/HopCount.hpp"
#include "message/ContentName.hpp"
#include "message/Content.hpp"
#include "config/Config.hpp"
#include "BuildProfile.hpp"
#include "interface/data_access/IRIB.hpp"

UseCaseInteractor::UseCaseInteractor(IForwardingInformationBase &fibRepository,
                                     IPendingInterestTable &pitRepository,
                                     IContentStore &csRepository,
                                     IRIB &ribRepository)
    : fibRepository(fibRepository), pitRepository(pitRepository),
      csRepository(csRepository), ribRepository(ribRepository)
{
}

/// @brief Interestパケットを受信したときの処理
/// @param inputData 入力された Interest データ構造
/// @return 応答パケット（DATA, INTEREST, INVALID）
OutputData UseCaseInteractor::handleInterestReceive(const InputData &inputData)
{
    SenderId senderId(inputData.senderId);
    DestinationId destinationId({inputData.destId});
    SignalCode signalCode = fromString(inputData.signalCode);
    HopCount hopcount(inputData.hopCount);
    ContentName contentName(inputData.contentName);
    Content content(inputData.content);

    // INTEREST受信時の処理
    // ホップカウントチェック（転送時の値で判定）
    if (hopcount.getValue() + 1 >= systemConfig.hopCountThreshold)
    {
        // パケット破棄
        return makeOutput(
            VALUE_NA,
            {VALUE_NA},
            toString(SignalCode::INVALID),
            hopcount.getValue() + 1,
            VALUE_NA,
            VALUE_NA);
    }

    if (systemConfig.maxCsTableSize > 0 && csRepository.find(contentName))
    {
        Content res = csRepository.get(contentName);
        // CSからデータ送信 (新しいDATAパケットなのでホップ数=0)
        return makeOutput(
            *destinationId.getValue().begin(),
            {senderId.getValue()},
            toString(SignalCode::DATA),
            0,
            contentName.getValue(),
            res.getValue());
    }
    else
    {
        // PITテーブルに保存
        PITPair pitPair(contentName, DestinationId({senderId.getValue()}));
        pitRepository.save(pitPair);

        if (fibRepository.find(contentName))
        {
            // FIBテーブルに基づいてINTEREST送信 (転送なのでホップ数+1)
            return makeOutput(
                *destinationId.getValue().begin(),
                fibRepository.get(contentName).getValue(),
                toString(SignalCode::INTEREST),
                hopcount.getValue() + 1,
                contentName.getValue(),
                content.getValue());
        }
        else
        {
            // FIB未ヒット時はブロードキャストせず破棄
            return makeOutput(
                VALUE_NA,
                {VALUE_NA},
                toString(SignalCode::INVALID),
                hopcount.getValue() + 1,
                VALUE_NA,
                VALUE_NA);
        }
    }
};

/// @brief Dataパケットを受信したときの処理
/// @param inputData 入力された Data データ構造
/// @return 応答パケット（DATA, INVALID）
OutputData UseCaseInteractor::handleDataReceive(const InputData &inputData)
{
    SenderId senderId(inputData.senderId);
    DestinationId destinationId({inputData.destId});
    SignalCode signalCode = fromString(inputData.signalCode);
    HopCount hopcount(inputData.hopCount);
    ContentName contentName(inputData.contentName);
    Content content(inputData.content);

    // DATA受信時の処理
    if (pitRepository.find(contentName.getValue()))
    {
        // CSにキャッシュ
        if (systemConfig.maxCsTableSize > 0) {
            CSPair csPair(contentName, content);
            csRepository.save(csPair);
        }

        // PITに基づいてデータ送信 (転送なのでホップ数+1)
        return makeOutput(
            *destinationId.getValue().begin(),
            pitRepository.get(contentName).getValue(),
            toString(SignalCode::DATA),
            hopcount.getValue() + 1,
            contentName.getValue(),
            content.getValue());
    }
    else
    {
        // パケット破棄 (対応するINTERESTがない)
        return makeOutput(
            VALUE_NA,
            {VALUE_NA},
            toString(SignalCode::INVALID),
            hopcount.getValue() + 1,
            VALUE_NA,
            VALUE_NA);
    }
};

/// @brief センサーデータを受信したときの処理
/// @details センサーデータはCSに保存されるだけで、応答は生成されない。
/// @param inputData 入力されたセンサーデータ構造
void UseCaseInteractor::handleSensorDataReceive(const InputData &inputData)
{
    ContentName contentName(inputData.contentName);
    Content content(inputData.content);
    if (systemConfig.maxCsTableSize > 0) {
        CSPair csPair(contentName, content);
        csRepository.save(csPair);
    }

    // csRepository.printCache();
}

/// @brief FIBに初期エントリを投入する
/// @param contentName コンテンツ名プレフィックス
/// @param nextHopMac 次ホップのMACアドレス文字列（小文字コロン区切り）
void UseCaseInteractor::initFIBEntry(const std::string &contentName, const std::string &nextHopMac)
{
    ribRepository.addRoute(contentName, nextHopMac);
}

/// @brief FIBの内容をシリアルに出力する
void UseCaseInteractor::printFIB() const
{
    fibRepository.printCache();
}

/// @brief Content Store をクリアする
void UseCaseInteractor::clearCSCache()
{
    csRepository.clear();
    CLI_PRINTLN("[CACHE] Content Store cleared");
}

/// @brief PIT をクリアする
void UseCaseInteractor::clearPITCache()
{
    pitRepository.clear();
    CLI_PRINTLN("[CACHE] PIT cleared");
}
