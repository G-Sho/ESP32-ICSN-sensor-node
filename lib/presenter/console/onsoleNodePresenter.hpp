#ifndef INCLUDED_CONSOLE_NODE_PRESENTER_hpp_
#define INCLUDED_CONSOLE_NODE_PRESENTER_hpp_

#include "node/NodePresenter.hpp"
#include "node/NodeOutputData.hpp" 
#include "console/ConsoleView.hpp"
#include "console/ConsoleViewModel.hpp"

class ConsoleNodePresenter : public NodePresenter
{
private:
  ConsoleView consoleView;

public: 
  void output(NodeOutputData outputData) override
  {
    String senderId = outputData.getSenderId();
    String destId = outputData.getDestId();
    String signalCode = outputData.getSignalCode();
    int hopCount = outputData.getHopCount();
    String contentName = outputData.getContentName();
    String content = outputData.getContent();

    ConsoleViewModel viewModel(senderId, destId, signalCode, hopCount, contentName, content);
    consoleView.showCreatedMessage(viewModel);
  };
};

#endif // INCLUDED_CONSOLE_NODE__PRESENTER_hpp_