#include "LRUPITRepository.hpp"
#include "../../config/Config.hpp"

void LRUPITRepository::save(const PITPair &pitPair) {
    const std::string &name = pitPair.getContentName().getValue();
    std::set<std::string> newDestinations = pitPair.getDestinationId().getValue();
    
    std::set<std::string> existingDestinations;
    if (cache.get(name, existingDestinations)) {
        newDestinations.insert(existingDestinations.begin(), existingDestinations.end());
    }
    
    cache.put(name, newDestinations);
}

void LRUPITRepository::remove(const ContentName &contentName) {
    const std::string &name = contentName.getValue();
    cache.remove(name);
    
    Serial.printf("Removed PIT entry: Key=%s\n", name.c_str());
}

bool LRUPITRepository::find(const ContentName &contentName) {
    return cache.contains(contentName.getValue());
}

DestinationId LRUPITRepository::get(const ContentName &contentName) {
    const std::string &name = contentName.getValue();
    std::set<std::string> destinations;
    
    if (cache.get(name, destinations)) {
        return DestinationId(destinations);
    }
    
    return DestinationId::Null();
}
