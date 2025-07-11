#ifndef INCLUDED_PIT_REPOSITORY_INTERFACE_hpp_
#define INCLUDED_PIT_REPOSITORY_INTERFACE_hpp_

#include "model/ICN/PITPair.hpp"
#include "model/message/ContentName.hpp"
#include "model/message/DestinationId.hpp"

class PITRepositoryInterface{
    virtual void save(const PITPair &pitPair) {};
    virtual void remove(const ContentName &contentName) {};
    virtual bool find(const ContentName &contentName);
    virtual DestinationId get(const ContentName &contentName);
};

#endif // INCLUDED_PIT_REPOSITORY_INTERFACE_hpp_