#ifndef INCLUDED_NODE__INTERACTOR_hpp_
#define INCLUDED_NODE__INTERACTOR_hpp_

#include "model/message/SenderId.hpp"
#include "model/message/DestinationId.hpp"
#include "model/message/SignalCode.hpp"
#include "model/message/HopCount.hpp"
#include "model/message/ContentName.hpp"
#include "model/message/Content.hpp"
// #include "stub/StubFIBRepository.hpp"
// #include "stub/StubPITRepository.hpp"
// #include "stub/StubCSRepository.hpp"
#include "node/NodeInputData.hpp"
#include "node/NodeOutputData.hpp"
#include "node/NodeUseCase.hpp"
#include "model/ICN/FIBPair.hpp"
#include "model/ICN/PITPair.hpp"
#include "model/ICN/CSPair.hpp"
#include "shonoshin/TwoStageLookupFIBRepository.hpp"
// #include "shonoshin/LRUPITRepository.hpp"
// #include "shonoshin/pCASTINGCSRepository.hpp"
#include "interface/pCASTINGCSRepository.hpp"
#include "interface/LRUPITRepository.hpp"
#include "interface/TwoStageLookupFIBRepository.hpp"
// #include "node/NodePresenter.h"
// #include "console/ConsoleNodePresenter.hpp"

// SIGNAL
#define SIGNAL_INTEREST "1"    // Interest
#define SIGNAL_DATA "2"        // Data
#define SIGANAL_INVALID "3"    // Invalid message
#define HOP_COUNT_THRESHOLD 16 // Hop Count Threshold

class NodeInteractor : public NodeUseCase
{
private:
  // StubFIBRepository fibRepository;
  // StubPITRepository pitRepository;
  // StubCSRepository csRepository;
  TwoStageLookupFIBRepository fibRepository;
  LRUPITRepository pitRepository;
  pCASTINGCSRepository csRepository;
  // NodePresenter nodePresenter;
  // ConsoleNodePresenter consoleNodePresenter;

public:
  void setMesh(painlessMesh *meshPtr)
  {
    csRepository.setMesh(meshPtr);
  };

  NodeOutputData handleInterestReceive(NodeInputData inputData) override
  {
    SenderId senderId(inputData.getSenderId());
    DestinationId destinationId({inputData.getDestId()});
    SignalCode signalCode(inputData.getSignalCode());
    HopCount hopcount(inputData.getHopCount());
    hopcount.increment();
    ContentName contentName(inputData.getContentName());
    Content content({inputData.getContent(), inputData.getTime()});

    // processing when receiving an Interest
    if (hopcount.getValue() >= HOP_COUNT_THRESHOLD)
    {
      // packet discard
      NodeOutputData outputData(
          std::string("NULL"),
          {std::string("NULL")},
          SIGANAL_INVALID,
          hopcount.getValue(),
          std::string("NULL"),
          std::string("NULL"),
          uint32_t(0));
      return outputData;
    }

    if (csRepository.find(contentName))
    {
      Content res = csRepository.get(contentName);
      // send data based on CS
      NodeOutputData outputData(
          *destinationId.getValue().begin(),
          {senderId.getValue()},
          SIGNAL_DATA,
          0,
          contentName.getValue(),
          res.getValue().first,
          res.getValue().second);
      return outputData;
    }
    else
    {
      // save to PIT Table
      PITPair pitPair(contentName, DestinationId({senderId.getValue()}));
      pitRepository.save(pitPair);

      if (fibRepository.find(contentName))
      {
        // send Interest based on FIB Table
        NodeOutputData outputData(
            *destinationId.getValue().begin(),
            fibRepository.get(contentName).getValue(),
            SIGNAL_INTEREST,
            hopcount.getValue(),
            contentName.getValue(),
            content.getValue().first,
            content.getValue().second);
        return outputData;
      }
      else
      {
        // broadcast Interest
        NodeOutputData outputData(
            *destinationId.getValue().begin(),
            {std::string("-1")},
            SIGNAL_INTEREST,
            hopcount.getValue(),
            contentName.getValue(),
            content.getValue().first,
            content.getValue().second);
        return outputData;
      }
    }
  };

  NodeOutputData handleDataReceive(NodeInputData inputData) override
  {
    SenderId senderId(inputData.getSenderId());
    DestinationId destinationId({inputData.getDestId()});
    SignalCode signalCode(inputData.getSignalCode());
    HopCount hopcount(inputData.getHopCount());
    hopcount.increment();
    ContentName contentName(inputData.getContentName());
    Content content({inputData.getContent(), inputData.getTime()});

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
      NodeOutputData outputData(
          *destinationId.getValue().begin(),
          pitRepository.get(contentName).getValue(),
          SIGNAL_DATA,
          hopcount.getValue(),
          contentName.getValue(),
          content.getValue().first,
          content.getValue().second);
      pitRepository.remove(contentName);
      return outputData;
    }
    else
    {
      // save to PIT Table
      FIBPair fibPair(contentName, DestinationId({senderId.getValue()}));
      fibRepository.save(fibPair);

      // packet discard
      NodeOutputData outputData(
          std::string("NULL"),
          {std::string("NULL")},
          SIGANAL_INVALID,
          hopcount.getValue(),
          std::string("NULL"),
          std::string("NULL"),
          uint32_t(0));
      return outputData;
    }
  };

  void handleSensorDataReceive(NodeInputData inputData) override
  {
    // ContentName contentName(inputData.getContentName());
    // Content content({inputData.getContent(), inputData.getTime()});
    // CSPair csPair(contentName, content);
    // csRepository.save(csPair);
    // // csRepository.printCache();

    // --- Test 1: Save & Find ---
    ContentName contentName(inputData.getContentName());
    Content content({inputData.getContent(), inputData.getTime()});
    CSPair csPair(contentName, content);
    csRepository.save(csPair);

    bool found = csRepository.find(contentName);
    Serial.printf("[Test1] key found: %s\n", found ? "success" : "failure");

    // --- Test 2: Get content ---
    Content result = csRepository.get(contentName);
    Serial.printf("[Test2] key content: %s\n", result.getValue().first.c_str());

    // --- Test 3: Remove content ---
    csRepository.remove(contentName);
    bool afterRemove = csRepository.find(contentName);
    Serial.printf("[Test3] key remove result: %s\n", afterRemove ? "failure" : "success");

    // --- Test 4: Expired entry handling ---
    ContentName name2("/Hoge/Expired");
    Content expiredContent({"expired_value", 0.0}); // 時間切れのエントリ
    CSPair expiredPair(name2, expiredContent);
    csRepository.save(expiredPair);

    // 更新後のキャッシュ状態確認
    csRepository.printCache();

    Serial.printf("=== All tests done ===\n");
  };
};

#endif // INCLUDED_NODE__INTERACTOR_hpp_