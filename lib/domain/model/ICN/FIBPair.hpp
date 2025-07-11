#ifndef INCLUDED_FIB_hpp_
#define INCLUDED_FIB_hpp_

#include "model/message/ContentName.hpp"
#include "model/message/DestinationId.hpp"

class FIBPair
{
private:
  ContentName contentName;
  DestinationId destinationId;

public:
  FIBPair(const ContentName& contentName, const DestinationId& destinationId)
  : contentName(contentName), destinationId(destinationId) {}

  ContentName getContentName() const { return contentName; };

  DestinationId getDestinationId() const { return destinationId; };
};

#endif // INCLUDED_FIB_hpp_