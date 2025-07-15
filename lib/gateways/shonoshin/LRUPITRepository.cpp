#include "interface/LRUPITRepository.hpp"

void LRUPITRepository::save(const PITPair &pitPair) {
    const std::string &name = pitPair.getContentName().getValue();
    std::set<std::string> preId = pitPair.getDestinationId().getValue();

    if (iter.count(name) != 0) {
        auto it = iter[name];
        std::set<std::string> oldSetCopy = it->second;
        std::cout << "Updating existing entry for: " << name << std::endl;

        preId.insert(oldSetCopy.begin(), oldSetCopy.end());
        Q.erase(it);
    }

    Q.push_front({name, preId});
    iter[name] = Q.begin();

    if (Q.size() > MAX_PIT_TABLE_SIZE) {
        auto pair = Q.back();
        const std::string &k = pair.first;
        iter.erase(k);
        Q.pop_back();
    }
}

void LRUPITRepository::remove(const ContentName &contentName) {
    const std::string &name = contentName.getValue();
    if (iter.count(name)) {
        Q.erase(iter[name]);
        iter.erase(name);
    }
}

bool LRUPITRepository::find(const ContentName &contentName) {
    return iter.count(contentName.getValue()) != 0;
}

DestinationId LRUPITRepository::get(const ContentName &contentName) {
    const std::string &name = contentName.getValue();

    if (iter.count(name) == 0)
        return DestinationId::Null();

    auto it = iter[name];
    std::set<std::string> v = it->second;
    Q.erase(it);
    Q.push_front({name, v});
    iter[name] = Q.begin();

    return DestinationId(v);
}
