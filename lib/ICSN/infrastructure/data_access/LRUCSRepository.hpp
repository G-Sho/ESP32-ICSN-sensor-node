#pragma once

#include "data_access/ICSRepository.hpp"
#include <unordered_map>
#include <list>
#include <set>
#include <string>
#include <iostream>
#include <Arduino.h>

class LRUCSRepository : public ICSRepository
{
private:
    std::list<std::tuple<std::string, std::string, uint32_t>> Q;                                               // {key, value}
    std::unordered_map<std::string, std::list<std::tuple<std::string, std::string, uint32_t>>::iterator> iter; // <key, iterator>
    
public:
    void save(const CSPair &pcsPair) override;
    void remove(const ContentName &contentName) override;
    bool find(const ContentName &contentName) override;
    Content get(const ContentName &contentName) override;

    void printCache() const
    {
        Serial.printf("=== Content Store ===\n");
        int index = 0;

        uint32_t now = millis();

        for (const auto &entry : Q)
        {
            const std::string &key = std::get<0>(entry);
            const std::string &value = std::get<1>(entry);
            uint32_t timestamp = std::get<2>(entry);
            
            // Print cache entry
            Serial.printf("[%d] Key: %s, Value: %s, Timestamp: %lu\n",
                          index++, key.c_str(), value.c_str(), timestamp);
        }

        Serial.printf("======================\n\n");
    }
};
