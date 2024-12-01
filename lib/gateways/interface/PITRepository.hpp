#ifndef INCLUDED_PIT_REPOSITORY_hpp_
#define INCLUDED_PIT_REPOSITORY_hpp_

#include "model/ICN/PITPair.hpp"
#include "model/message/ContentName.hpp"
#include "model/message/DestinationId.hpp"

class PITRepository{
    virtual void save(PITPair pitPair) {};
    virtual void remove(ContentName contentName) {};
    virtual bool find(ContentName contentName);
    virtual DestinationId get(ContentName contentName);
};

#endif // INCLUDED_PIT_REPOSITORY_hpp_