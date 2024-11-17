#ifndef INCLUDED_FIB_hpp_
#define INCLUDED_FIB_hpp_

#include "model\message\ContentName.hpp"
#include "model\message\DestinationId.hpp"

class FIBPair
{
private:
  ContentName contentName;
  DestinationId destinationId;

public:
  FIBPair(const ContentName& contentName, const DestinationId& destinationIdId)
  : contentName(contentName), destinationId(destinationId) {}

  ContentName getContentName() { return contentName; };

  DestinationId getDestinationId() { return destinationId; };
};

#endif // INCLUDED_FIB_hpp_