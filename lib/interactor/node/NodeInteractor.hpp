#ifndef INCLUDED_NODE__INTERACTOR_hpp_
#define INCLUDED_NODE__INTERACTOR_hpp_

#include "node\NodeUseCase.hpp"
#include "model\ICN\CS.hpp"
#include "stub\StubCSRepository.hpp"
#include "model\ICN\FIB.hpp"
// #include "stub\StubFIBRepository.hpp"
#include "fast\FastFIBRepository.hpp"
#include "model\ICN\PIT.hpp"
#include "stub\StubPITRepository.hpp"
// #include "node\NodePresenter.h"
// #include "console\ConsoleNodePresenter.hpp"
#include "node\NodeInputData.hpp"
#include "node\NodeOutputData.hpp"

// SIGNAL
#define SIGNAL_INTEREST '1' // Interest
#define SIGNAL_DATA '2'     // Data
#define SIGANAL_INVALID '3' // Invalid message

class NodeInteractor : public NodeUseCase
{
private:
  StubCSRepository csRepository;
  // StubFIBRepository fibRepository;
  FastFIBRepository fibRepository;
  StubPITRepository pitRepository;
  // NodePresenter nodePresenter;
  // ConsoleNodePresenter consoleNodePresenter;

public:
  NodeOutputData handleInterestReceive(NodeInputData inputData) override
  {
    // Interestを受信したときの処理
    if (inputData.getHopCount() >= 16)
    {
      // owari ni sasetai. packet haki sitai.
      NodeOutputData outputData(
          String("NULL"),
          String("NULL"),
          String(SIGANAL_INVALID),
          inputData.getHopCount() + 1,
          String("NULL"),
          String("NULL"));
      // SHUTURYOKU
      // consoleNodePresenter.output(outputData);
      return outputData;
    }

    if (csRepository.find(ContentName(inputData.getContentName())))
    { // if this node has the content
      // single
      NodeOutputData outputData(
          inputData.getDestId(),
          inputData.getSenderId(),
          String(SIGNAL_DATA),
          inputData.getHopCount() + 1,
          inputData.getContentName(),
          csRepository.get(ContentName(inputData.getContentName())).getValue());
      // consoleNodePresenter.output(outputData);
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
        NodeOutputData outputData(
            inputData.getDestId(),
            fibRepository.get(inputData.getContentName()).getValue(),
            String(SIGNAL_INTEREST),
            inputData.getHopCount() + 1,
            inputData.getContentName(),
            inputData.getContent());
        // consoleNodePresenter.output(outputData);
        return outputData;
      }
      else
      { // broadcast
        NodeOutputData outputData(
            inputData.getDestId(),
            String("-1"),
            String(SIGNAL_INTEREST),
            inputData.getHopCount() + 1,
            inputData.getContentName(),
            inputData.getContent());
        // SHUTURYOKU
        // consoleNodePresenter.output(outputData);
        return outputData;
      }
    }
  };

  NodeOutputData handleDataReceive(NodeInputData inputData) override
  {
    unsigned long previousMillis = millis();
    unsigned long currentMillis = 0;
    // Dataを受信したときの処理

    if (pitRepository.find(ContentName(inputData.getContentName())))
    {
      CS cs(ContentName(inputData.getContentName()), Content(inputData.getContent()));
      csRepository.save(cs);
      // single
      NodeOutputData outputData(
          inputData.getDestId(),
          pitRepository.get(inputData.getContentName()).getValue(),
          String(SIGNAL_DATA),
          inputData.getHopCount() + 1,
          inputData.getContentName(),
          inputData.getContent());
      // SHUTURYOKU
      // consoleNodePresenter.output(outputData);
      pitRepository.remove(ContentName(inputData.getContentName()));
      return outputData;
    }
    else
    {
      // owari ni sasetai. packet haki sitai.
      NodeOutputData outputData(
          String("NULL"),
          String("NULL"),
          String(SIGANAL_INVALID),
          inputData.getHopCount() + 1,
          String("NULL"),
          String("NULL"));
      // SHUTURYOKU
      // consoleNodePresenter.output(outputData);
      return outputData;
    }
  };

  void handleSensorDataReceive(NodeInputData inputData) override
  {
    CS cs(ContentName(inputData.getContentName()), Content(inputData.getContent()));
    csRepository.save(cs);
  };
};

#endif // INCLUDED_NODE__INTERACTOR_hpp_