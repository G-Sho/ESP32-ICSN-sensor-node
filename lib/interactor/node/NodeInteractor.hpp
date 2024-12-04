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
//#include "shonoshin/TwoStageLookupFIBRepository.hpp"
#include "yamamoto/DirectedDiffusionFIBRepository.hpp"
#include "shonoshin/FIFOPITRepository.hpp"
#include "shonoshin/FIFOCSRepository.hpp"
// #include "node/NodePresenter.h"
// #include "console/ConsoleNodePresenter.hpp"

// SIGNAL
#define SIGNAL_INTEREST "1" // Interest
#define SIGNAL_DATA "2"     // Data
#define SIGANAL_INVALID "3" // Invalid message

class NodeInteractor : public NodeUseCase
{
private:
  // StubFIBRepository fibRepository;
  // StubPITRepository pitRepository;
  // StubCSRepository csRepository;
  //TwoStageLookupFIBRepository fibRepository;
  DirectedDiffusionFIBRepository fibRepository;
  FIFOPITRepository pitRepository;
  FIFOCSRepository csRepository;
  // NodePresenter nodePresenter;
  // ConsoleNodePresenter consoleNodePresenter;

public:
  NodeOutputData handleInterestReceive(NodeInputData inputData) override
  {
    SenderId senderId(inputData.getSenderId());
    DestinationId destinationId({inputData.getDestId()});
    SignalCode signalCode(inputData.getSignalCode());
    HopCount hopcount(inputData.getHopCount());
    hopcount.increment();
    ContentName contentName(inputData.getContentName());
    Content content(inputData.getContent());

    // processing when receiving an Interest
    if (hopcount.getValue() >= 16)
    {
      // packet discard
      NodeOutputData outputData(
          std::string("NULL"),
          {std::string("NULL")},
          SIGANAL_INVALID,
          hopcount.getValue(),
          std::string("NULL"),
          std::string("NULL"));
      return outputData;
    }

    if (csRepository.find(contentName))
    {
      // send data based on CS
      NodeOutputData outputData(
          *destinationId.getValue().begin(),
          {senderId.getValue()},
          SIGNAL_DATA,
          0,
          contentName.getValue(),
          csRepository.get(contentName).getValue());
      return outputData;
    }
    else
    {/* //testuyou
      // save to PIT Table
      PITPair pitPair(contentName, DestinationId({senderId.getValue()}));
      pitRepository.save(pitPair);
      */
      if (fibRepository.find(contentName))
      {
        // send Interest based on FIB Table
        NodeOutputData outputData(
            *destinationId.getValue().begin(),
            fibRepository.get(contentName).getValue(),
            SIGNAL_INTEREST,
            hopcount.getValue(),
            contentName.getValue(),
            content.getValue());
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
            content.getValue());
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
    Content content(inputData.getContent());

    // processing when receiving an DARA
    if (pitRepository.find(contentName.getValue()))
    {
      // cache in CS
      CSPair csPair(contentName, content);
      csRepository.save(csPair);
      // send data based on PIT
      NodeOutputData outputData(
          *destinationId.getValue().begin(),
          pitRepository.get(contentName).getValue(),
          SIGNAL_DATA,
          hopcount.getValue(),
          contentName.getValue(),
          content.getValue());
      pitRepository.remove(contentName);
      return outputData;
    }
    else
    {
      // save to PIT Table
      FIBPair fibPair(contentName, DestinationId({senderId.getValue()}));
      fibRepository.saveForDynamic(fibPair, senderId.getValue(), hopcount.getValue());
      // packet discard
      NodeOutputData outputData(
          std::string("NULL"),
          {std::string("NULL")},
          SIGANAL_INVALID,
          hopcount.getValue(),
          std::string("NULL"),
          std::string("NULL"));
      return outputData;
    }
  };

  void handleSensorDataReceive(NodeInputData inputData) override
  {
    ContentName contentName(inputData.getContentName());
    Content content(inputData.getContent());
    CSPair csPair(contentName, content);
    csRepository.save(csPair);
  };
};

#endif // INCLUDED_NODE__INTERACTOR_hpp_