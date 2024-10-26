#ifndef INCLUDED_PIT_hpp_
#define INCLUDED_PIT_hpp_

#include "model\message\ContentName.hpp"
#include "model\message\NodeId.hpp"

class PIT
{
private:
  ContentName contentName;
  NodeId nodeId;

public:
  PIT(const ContentName& contentName, const NodeId& nodeId)
  : contentName(contentName), nodeId(nodeId) {}

  ContentName getContentName() { return contentName; };

  NodeId getNodeId() { return nodeId; };
};

#endif // INCLUDED_PIT_hpp_