#ifndef INCLUDED_FIB_REPOSITORY_hpp_
#define INCLUDED_FIB_REPOSITORY_hpp_

#include "model\ICN\FIB.hpp"
#include "model\message\ContentName.hpp"
#include "model\message\NodeId.hpp"

class FIBRepository{
    virtual void save(FIB fib) {};
    virtual void remove(ContentName contentName) {};
    virtual bool find(ContentName contentName);
    virtual NodeId get(ContentName contentName);
};

#endif // INCLUDED_FIB_REPOSITORY_hpp_