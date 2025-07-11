#ifndef INCLUDED_LRU_PIT_REPOSITORY_hpp_
#define INCLUDED_LRU_PIT_REPOSITORY_hpp_

#include "interface/PITRepositoryInterface.hpp"
#include <unordered_map>

#define MAX_PIT_TABLE_SIZE 20

class LRUPITRepository : public PITRepositoryInterface
{
private:
    std::list<std::pair<std::string, std::set<std::string>>> Q;                                               // {key, value}
    std::unordered_map<std::string, std::list<std::pair<std::string, std::set<std::string>>>::iterator> iter; // <key, iterator>

public:
    void save(const PITPair &pitPair) override
    {
        const std::string &name = pitPair.getContentName().getValue();
        std::set<std::string> preId = pitPair.getDestinationId().getValue();
        if (iter.count(name) != 0)
        {
            auto it = iter[name];
            preId.insert(it->second.begin(), it->second.end());
            Q.erase(iter[name]);
        };
        Q.push_front({name, pitPair.getDestinationId().getValue()});
        iter[name] = Q.begin();
        if (Q.size() > MAX_PIT_TABLE_SIZE)
        {
            auto pair = *(--Q.end());
            const std::string &k = pair.first;
            const std::set<std::string> &v = pair.second;
            iter.erase(k);
            Q.pop_back();
        }
    };

    void remove(const ContentName &contentName) override
    {
        const std::string &name = contentName.getValue();
        if (iter.count(name) != 0)
        {
            Q.erase(iter[name]);
            iter.erase(name);
        }
    };

    bool find(const ContentName &contentName) override
    {
        if (iter.count(contentName.getValue()) == 0)
            return false;
        else
            return true;
    };

    DestinationId get(const ContentName &contentName) override
    {
        const std::string &name = contentName.getValue();
        if (iter.count(name) == 0)
            return DestinationId::Null();

        auto it = iter[name];
        auto pair = *it;
        const std::string &k = pair.first;
        const std::set<std::string> &v = pair.second;
        Q.erase(it);
        Q.push_front({name, v});
        iter[name] = Q.begin();
        return DestinationId(v);
    };
};

#endif // INCLUDED_LRU_PIT_REPOSITORY_hpp_