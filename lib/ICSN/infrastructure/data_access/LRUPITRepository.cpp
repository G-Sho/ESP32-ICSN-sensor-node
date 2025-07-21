#include "LRUPITRepository.hpp"
#include "config/Config.hpp"

void LRUPITRepository::save(const PITPair &pitPair) {
    const std::string &name = pitPair.getContentName().getValue();
    std::set<std::string> preId = pitPair.getDestinationId().getValue();

    if (iter.count(name) != 0) {
        auto it = iter[name];
        std::set<std::string> oldSetCopy = it->second;
        preId.insert(oldSetCopy.begin(), oldSetCopy.end());
        Q.erase(it);
    }

    Q.push_front({name, preId});
    iter[name] = Q.begin();

    if (Q.size() > systemConfig.maxPitTableSize) {
        auto pair = Q.back();
        const std::string &k = pair.first;
        iter.erase(k);
        Q.pop_back();
    }

    // Print cache contents
    Serial.printf("Saved: Key=%s, Value: {", name.c_str());
    for (const auto &id : preId) {
        Serial.printf("%s ", id.c_str());
    }
    Serial.printf("}\n");
    printCache();
}

void LRUPITRepository::remove(const ContentName &contentName) {
    const std::string &name = contentName.getValue();
    if (iter.count(name)) {
        Q.erase(iter[name]);
        iter.erase(name);
    }

    // Print cache contents after removal
    Serial.printf("Removed: Key=%s\n", name.c_str());
    printCache();
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

    // Print cache contents after retrieval
    Serial.printf("Retrieved: Key=%s, Value: {", name.c_str());
    for (const auto &id : v) {
        Serial.printf("%s ", id.c_str());
    }
    Serial.printf("}\n");
    printCache();
    
    return DestinationId(v);
}
