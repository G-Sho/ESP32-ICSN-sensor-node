#include "LRUFIBRepository.hpp"
#include "../../config/Config.hpp"

void LRUFIBRepository::save(const FIBPair &fibPair) {
    const std::string &name = fibPair.getContentName().getValue();
    std::set<std::string> newDestinations = fibPair.getDestinationId().getValue();
    
    std::set<std::string> existingDestinations;
    if (cache.get(name, existingDestinations)) {
        newDestinations.insert(existingDestinations.begin(), existingDestinations.end());
    }
    
    cache.put(name, newDestinations);
}

void LRUFIBRepository::remove(const ContentName &contentName) {
    const std::string &name = contentName.getValue();
    cache.remove(name);
}

bool LRUFIBRepository::find(const ContentName &contentName) {
    return cache.contains(contentName.getValue());
}

DestinationId LRUFIBRepository::get(const ContentName &contentName) {
    const std::string &name = contentName.getValue();
    std::set<std::string> destinations;
    
    if (cache.get(name, destinations)) {
        return DestinationId(destinations);
    }
    
    return DestinationId::Null();
}
