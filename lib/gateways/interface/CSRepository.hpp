#pragma once

#include "model/ICN/CSPair.hpp"
#include "model/message/ContentName.hpp"
#include "model/message/Content.hpp"

class CSRepository
{
public:
    virtual void save(const CSPair &csPair) = 0;
    virtual void remove(const ContentName &contentName) = 0;
    virtual bool find(const ContentName &contentName) = 0;
    virtual Content get(const ContentName &contentName) = 0;

    virtual ~CSRepository() = default;
};