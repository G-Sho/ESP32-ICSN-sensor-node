#pragma once

#include "../../interface/data_access/IFIBRepository.hpp"
#include "FixedSizeLRUCache.hpp"
#include <set>
#include <string>
#include <Arduino.h>

class LRUFIBRepository : public IFIBRepository
{
private:
    static constexpr size_t MAX_FIB_SIZE = 20;
    FixedSizeLRUCache<std::set<std::string>, MAX_FIB_SIZE> cache;

public:
    void save(const FIBPair &fibPair) override;
    void remove(const ContentName &contentName) override;
    bool find(const ContentName &contentName) override;
    DestinationId get(const ContentName &contentName) override;

    void printCache() const
    {
        Serial.printf("=== Forwarding Information Base ===\n");
        cache.printCache();
    }
};
