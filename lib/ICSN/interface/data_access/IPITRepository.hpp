#pragma once

#include "routing_table/PITPair.hpp"
#include "message/ContentName.hpp"
#include "message/DestinationId.hpp"

class IPITRepository
{
public:
    virtual void save(const PITPair &pitPair) = 0;
    virtual void remove(const ContentName &contentName) = 0;
    virtual bool find(const ContentName &contentName) = 0;
    virtual DestinationId get(const ContentName &contentName) = 0;

    virtual ~IPITRepository() = default;
};
