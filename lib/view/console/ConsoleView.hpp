#ifndef INCLUDED_CONSOLE_VIEW_hpp_
#define INCLUDED_CONSOLE_VIEW_hpp_

#include "console/ConsoleViewModel.hpp"

class ConsoleView
{
public:
  void showCreatedMessage(ConsoleViewModel viewModel)
  {
    Serial.println("Message created");
    Serial.println("Sender Id: " + viewModel.getSenderId());
    Serial.println("Dest Id: " + viewModel.getDestId());
    Serial.println("Signal Code: " + viewModel.getSignalCode());
    Serial.println("Hop Count: " + String(viewModel.getHopCount()));
    Serial.println("Content Name: " + viewModel.getContentName());
    Serial.println("content: " + viewModel.getContent());
  };
};

#endif // INCLUDED_CONSOLE_VIEW_hpp_