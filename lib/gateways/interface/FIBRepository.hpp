#ifndef INCLUDED_FIB_REPOSITORY_hpp_
#define INCLUDED_FIB_REPOSITORY_hpp_

#include "model\ICN\FIBPair.hpp"
#include "model\message\ContentName.hpp"
#include "model\message\DestinationId.hpp"

class FIBRepository{
    virtual void save(FIBPair fibPair, int hopCount) {};
    virtual void remove(ContentName contentName) {};
    virtual bool find(ContentName contentName);
    virtual DestinationId get(ContentName contentName);
};

#endif // INCLUDED_FIB_REPOSITORY_hpp_