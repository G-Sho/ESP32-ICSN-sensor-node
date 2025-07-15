#pragma once

#include "model/ICN/FIBPair.hpp"
#include "model/message/ContentName.hpp"
#include "model/message/DestinationId.hpp"

class FIBRepository {
public:
    virtual void save(const FIBPair &fibPair) = 0;
    virtual void remove(const ContentName &contentName) = 0;
    virtual bool find(const ContentName &contentName) = 0;
    virtual DestinationId get(const ContentName &contentName) = 0;

    virtual ~FIBRepository() = default;
};