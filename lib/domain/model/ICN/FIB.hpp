#ifndef INCLUDED_FIB_hpp_
#define INCLUDED_FIB_hpp_

#include "model\message\ContentName.hpp"
#include "model\message\NodeId.hpp"

class FIB
{
private:
  ContentName contentName;
  NodeId nodeId;

public:
  FIB(const ContentName& contentName, const NodeId& nodeId)
  : contentName(contentName), nodeId(nodeId) {}

  ContentName getContentName() { return contentName; };

  NodeId getNodeId() { return nodeId; };
};

#endif // INCLUDED_FIB_hpp_