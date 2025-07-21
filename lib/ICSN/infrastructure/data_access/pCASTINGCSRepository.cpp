#include "pCASTINGCSRepository.hpp"
#include "config/Config.hpp"

void pCASTINGCSRepository::save(const CSPair &csPair)
{
    removeExpiredEntries();

    const std::string &name = csPair.getContentName().getValue();
    const std::string &content = csPair.getContent().getValue().first;
    const uint32_t &timeStamp = csPair.getContent().getValue().second;
    double EN = 1.0;
    double OC = (double)Q.size() / (double)systemConfig.maxCsTableSize;
    double FR = 1.0 - ((mesh->getNodeTime() - timeStamp) / systemConfig.cacheEntryTtlUs);

    if (pCASTING(EN, OC, FR))
    {
        if (iter.count(name) != 0)
        {
            Q.erase(iter[name]);
        }
        else if (Q.size() >= systemConfig.maxCsTableSize)
        {
            auto it = --Q.end();
            const std::string &k = std::get<0>(*it);
            iter.erase(k);
            Q.pop_back();
        }
        Q.push_front({name, content, timeStamp});
        iter[name] = Q.begin();
    }

    // Print cache contents
    Serial.printf("Saved: Key=%s, Value=%s, Timestamp=%lu\n", name.c_str(), content.c_str(), timeStamp);
    printCache();
}

void pCASTINGCSRepository::remove(const ContentName &contentName)
{
    const std::string &name = contentName.getValue();
    if (iter.count(name) != 0)
    {
        Q.erase(iter[name]);
        iter.erase(name);
    }

    // Print cache contents after removal
    Serial.printf("Removed: Key=%s\n", name.c_str());
    printCache();
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

    // Print cache contents after retrieval
    Serial.printf("Retrieved: Key=%s, Value=%s, Timestamp=%.2f\n", name.c_str(), v.c_str(), t);
    printCache();
}