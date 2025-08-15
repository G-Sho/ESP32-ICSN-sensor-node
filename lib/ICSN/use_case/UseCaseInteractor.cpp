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
    hopcount.increment();
    ContentName contentName(inputData.contentName);
    Content content(inputData.content);

    // processing when receiving an Interest
    if (hopcount.getValue() >= systemConfig.hopCountThreshold)
    {
        // packet discard
        return makeOutput(
            VALUE_NA,
            {VALUE_NA},
            toString(SignalCode::INVALID),
            hopcount.getValue(),
            VALUE_NA,
            VALUE_NA);
    }

    if (csRepository.find(contentName))
    {
        Content res = csRepository.get(contentName);
        // send data based on CS
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
        // save to PIT Table
        PITPair pitPair(contentName, DestinationId({senderId.getValue()}));
        pitRepository.save(pitPair);

        if (fibRepository.find(contentName))
        {
            // send Interest based on FIB Table
            return makeOutput(
                *destinationId.getValue().begin(),
                fibRepository.get(contentName).getValue(),
                toString(SignalCode::INTEREST),
                hopcount.getValue(),
                contentName.getValue(),
                content.getValue());
        }
        else
        {
            // broadcast Interest
            return makeOutput(
                *destinationId.getValue().begin(),
                {DEST_BROADCAST},
                toString(SignalCode::INTEREST),
                hopcount.getValue(),
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
    hopcount.increment();
    ContentName contentName(inputData.contentName);
    Content content(inputData.content);

    // processing when receiving an DATA
    if (pitRepository.find(contentName.getValue()))
    {
        // cache in CS
        CSPair csPair(contentName, content);
        csRepository.save(csPair);

        // cache in FIB
        FIBPair fibPair(contentName, DestinationId({senderId.getValue()}));
        fibRepository.save(fibPair);

        // send data based on PIT
        return makeOutput(
            *destinationId.getValue().begin(),
            pitRepository.get(contentName).getValue(),
            toString(SignalCode::DATA),
            hopcount.getValue(),
            contentName.getValue(),
            content.getValue());
    }
    else
    {
        // save to PIT Table
        FIBPair fibPair(contentName, DestinationId({senderId.getValue()}));
        fibRepository.save(fibPair);

        // packet discard
        return makeOutput(
            VALUE_NA,
            {VALUE_NA},
            toString(SignalCode::INVALID),
            hopcount.getValue(),
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
