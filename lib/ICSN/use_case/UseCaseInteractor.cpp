#include "UseCaseInteractor.hpp"
#include "message/SenderId.hpp"
#include "message/DestinationId.hpp"
#include "message/SignalCode.hpp"
#include "message/HopCount.hpp"
#include "message/ContentName.hpp"
#include "message/Content.hpp"
#include "config/Config.hpp"

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

    if (csRepository.find(contentName))
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
            // INTERESTブロードキャスト (転送なのでホップ数+1)
            return makeOutput(
                *destinationId.getValue().begin(),
                {DEST_BROADCAST},
                toString(SignalCode::INTEREST),
                hopcount.getValue() + 1,
                contentName.getValue(),
                content.getValue());
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
        CSPair csPair(contentName, content);
        csRepository.save(csPair);

        // FIBにキャッシュ
        FIBPair fibPair(contentName, DestinationId({senderId.getValue()}));
        fibRepository.save(fibPair);

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
        // 将来の使用のためFIBテーブルに保存
        FIBPair fibPair(contentName, DestinationId({senderId.getValue()}));
        fibRepository.save(fibPair);

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
    CSPair csPair(contentName, content);
    csRepository.save(csPair);

    // csRepository.printCache();
}

/// @brief FIBに初期エントリを投入する
/// @param contentName コンテンツ名プレフィックス
/// @param nextHopMac 次ホップのMACアドレス文字列（小文字コロン区切り）
void UseCaseInteractor::initFIBEntry(const std::string& contentName, const std::string& nextHopMac)
{
    ContentName name(contentName);
    FIBPair fibPair(name, DestinationId({nextHopMac}));
    fibRepository.save(fibPair);
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
    Serial.println("[CACHE] Content Store cleared");
}

/// @brief PIT をクリアする
void UseCaseInteractor::clearPITCache()
{
    pitRepository.clear();
    Serial.println("[CACHE] PIT cleared");
}

#ifdef UNIT_TEST
void UseCaseInteractor::mockAddToCS(const std::string &name, const std::string &content)
{
    ContentName contentName(name);
    Content contentObj(content);
    CSPair pair(contentName, contentObj);
    csRepository.save(pair);
}

void UseCaseInteractor::mockAddToFIB(const std::string &name, uint32_t nextHop)
{
    ContentName contentName(name);
    DestinationId dest({std::to_string(nextHop)});
    FIBPair pair(contentName, dest);
    fibRepository.save(pair);
}

void UseCaseInteractor::mockAddToPIT(const std::string &name, uint32_t fromNode)
{
    ContentName contentName(name);
    DestinationId dest({std::to_string(fromNode)});
    PITPair pair(contentName, dest);
    pitRepository.save(pair);
}
#endif
