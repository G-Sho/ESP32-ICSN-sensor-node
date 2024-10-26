#ifndef INCLUDED_NODE_ADD_USE_CASE_hpp_
#define INCLUDED_NODE_ADD_USE_CASE_hpp_

#include "NodeAddInputData.hpp"
#include "NodeAddOutputData.hpp"

class NodeAddUseCase
{
  virtual NodeAddOutputData handleInterestReceive(NodeAddInputData inputData);
  virtual NodeAddOutputData handleDataReceive(NodeAddInputData inputData);
  virtual void handleSensorDataReceive(NodeAddInputData inputData);
};

#endif // INCLUDED_NODE_ADD_USE_CASE_hpp_