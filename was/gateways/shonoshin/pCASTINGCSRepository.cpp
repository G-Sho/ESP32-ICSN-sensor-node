#include "interface/pCASTINGCSRepository.hpp"

void pCASTINGCSRepository::save(const CSPair &csPair)
{
    removeExpiredEntries();

    const std::string &name = csPair.getContentName().getValue();
    const std::string &content = csPair.getContent().getValue().first;
    const uint32_t &timeStamp = csPair.getContent().getValue().second;
    double EN = 1.0;
    double OC = Q.size() / MAX_CS_TABLE_SIZE;
    double FR = 1.0 - ((mesh->getNodeTime() - timeStamp) / CACHE_ENTRY_TTL_US);

    if (pCASTING(EN, OC, FR))
    {
        if (iter.count(name) != 0)
        {
            Q.erase(iter[name]);
        }
        else if (Q.size() >= MAX_CS_TABLE_SIZE)
        {
            auto it = --Q.end();
            const std::string &k = std::get<0>(*it);
            iter.erase(k);
            Q.pop_back();
        }
        Q.push_front({name, content, timeStamp});
        iter[name] = Q.begin();
    }
}

void pCASTINGCSRepository::remove(const ContentName &contentName)
{
    const std::string &name = contentName.getValue();
    if (iter.count(name) != 0)
    {
        Q.erase(iter[name]);
        iter.erase(name);
    }
}

bool pCASTINGCSRepository::find(const ContentName &contentName)
{
    return iter.count(contentName.getValue()) != 0;
}

Content pCASTINGCSRepository::get(const ContentName &contentName)
{
    removeExpiredEntries();

    const std::string &name = contentName.getValue();
    if (iter.count(name) == 0)
        return Content::Null();

    auto it = iter[name];
    const std::string &v = std::get<1>(*it);
    double t = std::get<2>(*it);
    Q.erase(it);
    Q.push_front({name, v, t});
    iter[name] = Q.begin();
    return Content({v, t});
}