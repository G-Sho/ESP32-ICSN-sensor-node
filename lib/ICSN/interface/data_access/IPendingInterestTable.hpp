#pragma once

#include "../entity/routing_table/PITPair.hpp"
#include "../entity/message/ContentName.hpp"
#include "../entity/message/DestinationId.hpp"

class IPendingInterestTable
{
public:
    virtual void save(const PITPair &pitPair) = 0;
    virtual void remove(const ContentName &contentName) = 0;
    virtual bool find(const ContentName &contentName) = 0;
    virtual DestinationId get(const ContentName &contentName) = 0;
    virtual void clear() = 0;
    virtual void setActiveSize(size_t size) = 0;
    virtual size_t getActiveSize() const = 0;

    virtual ~IPendingInterestTable() = default;
};
