#ifndef INCLUDED_NODE_USE_CASE_hpp_
#define INCLUDED_NODE_USE_CASE_hpp_

#include "NodeInputData.hpp"
#include "NodeOutputData.hpp"

class NodeUseCase
{
  virtual NodeOutputData handleInterestReceive(NodeInputData inputData);
  virtual NodeOutputData handleDataReceive(NodeInputData inputData);
  virtual void handleSensorDataReceive(NodeInputData inputData);
};

#endif // INCLUDED_NODE_USE_CASE_hpp_