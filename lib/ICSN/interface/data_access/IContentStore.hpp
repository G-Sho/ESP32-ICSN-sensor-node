#pragma once

#include "../entity/routing_table/CSPair.hpp"
#include "../entity/message/ContentName.hpp"
#include "../entity/message/Content.hpp"

class IContentStore
{
public:
    virtual void save(const CSPair &csPair) = 0;
    virtual void remove(const ContentName &contentName) = 0;
    virtual bool find(const ContentName &contentName) = 0;
    virtual Content get(const ContentName &contentName) = 0;

    virtual ~IContentStore() = default;
};
