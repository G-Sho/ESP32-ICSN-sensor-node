#ifndef INCLUDED_NODE__INTERACTOR_hpp_
#define INCLUDED_NODE__INTERACTOR_hpp_

#include "model/message/SenderId.hpp"
#include "model/message/DestinationId.hpp"
#include "model/message/SignalCode.hpp"
#include "model/message/HopCount.hpp"
#include "model/message/ContentName.hpp"
#include "model/message/Content.hpp"

#include "node\NodeUseCase.hpp"
#include "model\ICN\CSPair.hpp"
#include "stub\StubCSRepository.hpp"
#include "model\ICN\FIBPair.hpp"
#include "stub\StubFIBRepository.hpp"
// #include "fast\FastFIBRepository.hpp"
#include "model\ICN/PITPair.hpp"
#include "stub\StubPITRepository.hpp"
// #include "node\NodePresenter.h"
// #include "console\ConsoleNodePresenter.hpp"
#include "node\NodeInputData.hpp"
#include "node\NodeOutputData.hpp"

// SIGNAL
#define SIGNAL_INTEREST "1" // Interest
#define SIGNAL_DATA "2"     // Data
#define SIGANAL_INVALID "3" // Invalid message

class NodeInteractor : public NodeUseCase
{
private:
  StubCSRepository csRepository;
  StubFIBRepository fibRepository;
  // FastFIBRepository fibRepository;
  StubPITRepository pitRepository;
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

    // Interestを受信したときの処理
    if (hopcount.getValue() >= 16)
    {
      // owari ni sasetai. packet haki sitai.
      NodeOutputData outputData(
          std::string("NULL"),
          {std::string("NULL")},
          SIGANAL_INVALID,
          hopcount.getValue(),
          std::string("NULL"),
          std::string("NULL"));
      // SHUTURYOKU
      // consoleNodePresenter.output(outputData);
      return outputData;
    }

    if (csRepository.find(contentName))
    { // if this node has the content
      // single
      NodeOutputData outputData(
          destinationId.getValue()[0],
          {senderId.getValue()},
          SIGNAL_DATA,
          0,
          contentName.getValue(),
          csRepository.get(contentName).getValue());
      // consoleNodePresenter.output(outputData);
      return outputData;
    }
    else
    {
      PITPair pitPair(contentName, DestinationId({senderId.getValue()}));
      pitRepository.save(pitPair);
      if (fibRepository.find(contentName))
      { // single or multi
        NodeOutputData outputData(
            destinationId.getValue()[0],
            fibRepository.get(contentName).getValue(),
            SIGNAL_INTEREST,
            hopcount.getValue(),
            contentName.getValue(),
            content.getValue());
        // consoleNodePresenter.output(outputData);
        return outputData;
      }
      else
      { // broadcast
        NodeOutputData outputData(
            destinationId.getValue()[0],
            {std::string("-1")},
            SIGNAL_INTEREST,
            hopcount.getValue(),
            contentName.getValue(),
            content.getValue());
        // SHUTURYOKU
        // consoleNodePresenter.output(outputData);
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
    
    // Dataを受信したときの処理
    if (pitRepository.find(contentName.getValue()))
    {
      CSPair csPair(contentName, content);
      csRepository.save(csPair);
      // single
      NodeOutputData outputData(
          destinationId.getValue()[0],
          pitRepository.get(contentName).getValue(),
          SIGNAL_DATA,
          hopcount.getValue(),
          contentName.getValue(),
          content.getValue());
      // SHUTURYOKU
      // consoleNodePresenter.output(outputData);
      pitRepository.remove(contentName);
      return outputData;
    }
    else
    {
      // owari ni sasetai. packet haki sitai.
      NodeOutputData outputData(
          std::string("NULL"),
          {std::string("NULL")},
          SIGANAL_INVALID,
          hopcount.getValue(),
          std::string("NULL"),
          std::string("NULL"));
      // SHUTURYOKU
      // consoleNodePresenter.output(outputData);
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