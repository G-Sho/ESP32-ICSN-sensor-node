#pragma once

#include "../../interface/data_access/IPITRepository.hpp"
#include "FixedSizeLRUCache.hpp"
#include <set>
#include <string>
#include <Arduino.h>

class LRUPITRepository : public IPITRepository
{
private:
    static constexpr size_t MAX_PIT_SIZE = 20;
    FixedSizeLRUCache<std::set<std::string>, MAX_PIT_SIZE> cache;

public:
    void save(const PITPair &pitPair) override;
    void remove(const ContentName &contentName) override;
    bool find(const ContentName &contentName) override;
    DestinationId get(const ContentName &contentName) override;

    void printCache() const
    {
        Serial.printf("=== Pending Interest Table ===\n");
        cache.printCache();
    }
};
