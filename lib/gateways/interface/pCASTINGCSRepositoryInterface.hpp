#ifndef INCLUDED_pCASTING_CS_REPOSITORY_INTERFACE_hpp_
#define INCLUDED_pCASTING_CS_REPOSITORY_INTERFACE_hpp_

#include "model/ICN/CSPair.hpp"
#include "model/message/ContentName.hpp"
#include "model/message/Content.hpp"

class pCASTINGCSRepositoryInterface{
    virtual void save(const CSPair &csPair, const double &EN, const double &OC, const double &FR) {};
    virtual void remove(const ContentName &contentName) {};
    virtual bool find(const ContentName &contentName);
    virtual Content get(const ContentName &contentName);
};

#endif // INCLUDED_pCASTING_CS_REPOSIORY_INTERFACE_hpp_