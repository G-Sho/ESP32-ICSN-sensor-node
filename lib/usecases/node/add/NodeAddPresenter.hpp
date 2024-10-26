#ifndef INCLUDED_NODE_ADD_PRESENTER_hpp_
#define INCLUDED_NODE_ADD_PRESENTER_hpp_

#include "NodeAddOutputData.hpp"

class NodeAddPresenter
{
  virtual void output(NodeAddOutputData outputData) {};
  virtual void outputSingle(NodeAddOutputData outputData, String nodeId) {};
  virtual void outputBroadcast(NodeAddOutputData outputData) {};
};

#endif // INCLUDED_NODE_ADD_PRESENTER_hpp_