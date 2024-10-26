#ifndef INCLUDED_CONSOLE_NODE_ADD_PRESENTER_hpp_
#define INCLUDED_CONSOLE_NODE_ADD_PRESENTER_hpp_

#include "node\add\NodeAddPresenter.hpp"
#include "node\add\NodeAddOutputData.hpp" 
#include "console\ConsoleView.hpp"
#include "console\ConsoleViewModel.hpp"

class ConsoleNodeAddPresenter : public NodeAddPresenter
{
private:
  ConsoleView consoleView;

public: 
  void output(NodeAddOutputData outputData) override
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

#endif // INCLUDED_CONSOLE_NODE_ADD_PRESENTER_hpp_