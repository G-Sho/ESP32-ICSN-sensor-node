#include "LRUPendingInterestTable.hpp"
#include "../../config/Config.hpp"
#include "../../BuildProfile.hpp"

void LRUPendingInterestTable::save(const PITPair &pitPair) {
    const std::string &name = pitPair.getContentName().getValue();
    std::set<std::string> newDestinations = pitPair.getDestinationId().getValue();
    
    std::set<std::string> existingDestinations;
    if (cache.get(name, existingDestinations)) {
        newDestinations.insert(existingDestinations.begin(), existingDestinations.end());
    }
    
    cache.put(name, newDestinations);
}

void LRUPendingInterestTable::remove(const ContentName &contentName) {
    const std::string &name = contentName.getValue();
    cache.remove(name);
    
    LOG_DEBUGF("Removed PIT entry: Key=%s\n", name.c_str());
}

bool LRUPendingInterestTable::find(const ContentName &contentName) {
    return cache.contains(contentName.getValue());
}

DestinationId LRUPendingInterestTable::get(const ContentName &contentName) {
    const std::string &name = contentName.getValue();
    std::set<std::string> destinations;
    
    if (cache.get(name, destinations)) {
        return DestinationId(destinations);
    }
    
    return DestinationId::Null();
}
