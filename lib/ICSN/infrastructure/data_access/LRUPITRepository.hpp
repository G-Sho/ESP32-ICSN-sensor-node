#pragma once

#include "../../interface/data_access/IPITRepository.hpp"
#include "FixedSizeLRUCache.hpp"
#include "../../config/Config.hpp"
#include <set>
#include <string>
#include <Arduino.h>

class LRUPITRepository : public IPITRepository
{
private:
    FixedSizeLRUCache<std::set<std::string>, MAX_PIT_TABLE_SIZE> cache;

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
