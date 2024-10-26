#ifndef INCLUDED_NODE_ADD_INTERACTOR_hpp_
#define INCLUDED_NODE_ADD_INTERACTOR_hpp_

#include "node\add\NodeAddUseCase.hpp"
#include "model\ICN\CS.hpp"
#include "stub\StubCSRepository.hpp"
#include "model\ICN\FIB.hpp"
// #include "stub\StubFIBRepository.hpp"
#include "fast\FastFIBRepository.hpp"
#include "model\ICN\PIT.hpp"
#include "stub\StubPITRepository.hpp"
// #include "node\add\NodeAddPresenter.h"
// #include "console\ConsoleNodeAddPresenter.hpp"
#include "node\add\NodeAddInputData.hpp"
#include "node\add\NodeAddOutputData.hpp"

// SIGNAL
#define SIGNAL_INTEREST '1' // Interest
#define SIGNAL_DATA '2'     // Data
#define SIGANAL_INVALID '3' // Invalid message

class NodeAddInteractor : public NodeAddUseCase
{
private:
  StubCSRepository csRepository;
  // StubFIBRepository fibRepository;
  FastFIBRepository fibRepository;
  StubPITRepository pitRepository;
  // NodeAddPresenter nodeAddPresenter;
  // ConsoleNodeAddPresenter consoleNodeAddPresenter;

public:
  NodeAddOutputData handleInterestReceive(NodeAddInputData inputData) override
  {
    // Interestを受信したときの処理
    if (inputData.getHopCount() >= 16)
    {
      // owari ni sasetai. packet haki sitai.
      NodeAddOutputData outputData(
          String("NULL"),
          String("NULL"),
          String(SIGANAL_INVALID),
          inputData.getHopCount() + 1,
          String("NULL"),
          String("NULL"));
      // SHUTURYOKU
      // consoleNodeAddPresenter.output(outputData);
      return outputData;
    }

    if (csRepository.find(ContentName(inputData.getContentName())))
    { // if this node has the content
      // single
      NodeAddOutputData outputData(
          inputData.getDestId(),
          inputData.getSenderId(),
          String(SIGNAL_DATA),
          inputData.getHopCount() + 1,
          inputData.getContentName(),
          csRepository.get(ContentName(inputData.getContentName())).getValue());
      // consoleNodeAddPresenter.output(outputData);
      return outputData;
    }
    else
    {
      PIT pit(
        ContentName(inputData.getContentName()), 
        NodeId(inputData.getSenderId()));
      pitRepository.save(pit);
      if (fibRepository.find(inputData.getContentName()))
      { // single
        NodeAddOutputData outputData(
            inputData.getDestId(),
            fibRepository.get(inputData.getContentName()).getValue(),
            String(SIGNAL_INTEREST),
            inputData.getHopCount() + 1,
            inputData.getContentName(),
            inputData.getContent());
        // consoleNodeAddPresenter.output(outputData);
        return outputData;
      }
      else
      { // broadcast
        NodeAddOutputData outputData(
            inputData.getDestId(),
            String("-1"),
            String(SIGNAL_INTEREST),
            inputData.getHopCount() + 1,
            inputData.getContentName(),
            inputData.getContent());
        // SHUTURYOKU
        // consoleNodeAddPresenter.output(outputData);
        return outputData;
      }
    }
  };

  NodeAddOutputData handleDataReceive(NodeAddInputData inputData) override
  {
    unsigned long previousMillis = millis();
    unsigned long currentMillis = 0;
    // Dataを受信したときの処理

    if (pitRepository.find(ContentName(inputData.getContentName())))
    {
      CS cs(ContentName(inputData.getContentName()), Content(inputData.getContent()));
      csRepository.save(cs);
      // single
      NodeAddOutputData outputData(
          inputData.getDestId(),
          pitRepository.get(inputData.getContentName()).getValue(),
          String(SIGNAL_DATA),
          inputData.getHopCount() + 1,
          inputData.getContentName(),
          inputData.getContent());
      // SHUTURYOKU
      // consoleNodeAddPresenter.output(outputData);
      pitRepository.remove(ContentName(inputData.getContentName()));
      return outputData;
    }
    else
    {
      // owari ni sasetai. packet haki sitai.
      NodeAddOutputData outputData(
          String("NULL"),
          String("NULL"),
          String(SIGANAL_INVALID),
          inputData.getHopCount() + 1,
          String("NULL"),
          String("NULL"));
      // SHUTURYOKU
      // consoleNodeAddPresenter.output(outputData);
      return outputData;
    }
  };

  void handleSensorDataReceive(NodeAddInputData inputData) override
  {
    CS cs(ContentName(inputData.getContentName()), Content(inputData.getContent()));
    csRepository.save(cs);
  };
};

#endif // INCLUDED_NODE_ADD_INTERACTOR_hpp_