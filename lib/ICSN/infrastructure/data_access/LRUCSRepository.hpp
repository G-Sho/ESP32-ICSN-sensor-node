#pragma once

#include "../../interface/data_access/ICSRepository.hpp"
#include "FixedSizeLRUCache.hpp"
#include <string>
#include <Arduino.h>

class LRUCSRepository : public ICSRepository
{
private:
    static constexpr size_t MAX_CS_SIZE = 20;
    FixedSizeLRUCache<std::string, MAX_CS_SIZE> cache;

public:
    void save(const CSPair &csPair) override;
    void remove(const ContentName &contentName) override;
    bool find(const ContentName &contentName) override;
    Content get(const ContentName &contentName) override;

    void printCache() const
    {
        Serial.printf("=== Content Store ===\n");
        cache.printCache();
    }
};
