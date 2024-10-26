#ifndef INCLUDED_PIT_REPOSITORY_hpp_
#define INCLUDED_PIT_REPOSITORY_hpp_

#include "model\ICN\PIT.hpp"
#include "model\message\ContentName.hpp"
#include "model\message\NodeId.hpp"

class PITRepository{
    virtual void save(PIT pit) {};
    virtual void remove(ContentName contentName) {};
    virtual bool find(ContentName contentName);
    virtual NodeId get(ContentName contentName);
};

#endif // INCLUDED_PIT_REPOSITORY_hpp_