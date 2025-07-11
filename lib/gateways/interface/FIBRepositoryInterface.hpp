#ifndef INCLUDED_FIB_REPOSITORY_INTERFACE_hpp_
#define INCLUDED_FIB_REPOSITORY_INTERFACE_hpp_

#include "model/ICN/FIBPair.hpp"
#include "model/message/ContentName.hpp"
#include "model/message/DestinationId.hpp"

class FIBRepositoryInterface{
    virtual void save(const FIBPair &fibPair) {};
    virtual void remove(const ContentName &contentName) {};
    virtual bool find(const ContentName &contentName);
    virtual DestinationId get(const ContentName &contentName);
};

#endif // INCLUDED_FIB_REPOSITORY_INTERFACE_hpp_