#ifndef INCLUDED_CS_REPOSITORY_hpp_
#define INCLUDED_CS_REPOSITORY_hpp_

#include "model\ICN\CS.hpp"
#include "model\message\ContentName.hpp"
#include "model\message\Content.hpp"

class CSRepository{
    virtual void save(CS cs) {};
    virtual void remove(ContentName contentName) {};
    virtual bool find(ContentName contentName);
    virtual Content get(ContentName contentName);
};

#endif // INCLUDED_CS_REPOSIORY_hpp_