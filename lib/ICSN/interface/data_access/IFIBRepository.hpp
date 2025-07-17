#pragma once

#include "entity/routing_table/FIBPair.hpp"
#include "entity/message/ContentName.hpp"
#include "entity/message/DestinationId.hpp"

class IFIBRepository {
public:
    virtual void save(const FIBPair &fibPair) = 0;
    virtual void remove(const ContentName &contentName) = 0;
    virtual bool find(const ContentName &contentName) = 0;
    virtual DestinationId get(const ContentName &contentName) = 0;

    virtual ~IFIBRepository() = default;
};