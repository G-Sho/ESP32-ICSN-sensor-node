#pragma once

#include "message/ContentName.hpp"
#include "message/DestinationId.hpp"

class PITPair {
    ContentName name;
    DestinationId destId;
public:
    PITPair(const ContentName &c, const DestinationId &d) : name(c), destId(d) {}
    const ContentName& getContentName() const { return name; }
    const DestinationId& getDestinationId() const { return destId; }
};