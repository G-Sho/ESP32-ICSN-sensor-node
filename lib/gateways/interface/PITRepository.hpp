#pragma once

#include "model/ICN/PITPair.hpp"
#include "model/message/DestinationId.hpp"
#include "model/message/ContentName.hpp"

class PITRepository {
public:
    virtual void save(const PITPair &pitPair) = 0;
    virtual void remove(const ContentName &contentName) = 0;
    virtual bool find(const ContentName &contentName) = 0;
    virtual DestinationId get(const ContentName &contentName) = 0;

    virtual ~PITRepository() = default;
};
