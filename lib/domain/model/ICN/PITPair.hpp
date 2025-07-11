#ifndef INCLUDED_PIT_PAIR_hpp_
#define INCLUDED_PIT_PAIR_hpp_

#include "model/message/ContentName.hpp"
#include "model/message/DestinationId.hpp"

class PITPair
{
private:
  ContentName contentName;
  DestinationId destinationId;

public:
  PITPair(const ContentName& contentName, const DestinationId& destinationId)
  : contentName(contentName), destinationId(destinationId) {}

  ContentName getContentName() const { return contentName; };

  DestinationId getDestinationId() const { return destinationId; };
};

#endif // INCLUDED_PIT_PAIR_hpp_