#pragma once

#include "data_access/IFIBRepository.hpp"
#include <unordered_map>
#include <list>
#include <set>
#include <string>
#include <iostream>
#include <Arduino.h>

class LRUFIBRepository : public IFIBRepository
{
private:
    std::list<std::pair<std::string, std::set<std::string>>> Q;
    std::unordered_map<std::string, std::list<std::pair<std::string, std::set<std::string>>>::iterator> iter;

public:
    void save(const FIBPair &fibPair) override;
    void remove(const ContentName &contentName) override;
    bool find(const ContentName &contentName) override;
    DestinationId get(const ContentName &contentName) override;

    void printCache() const
    {
        Serial.printf("=== Forwarding InFormation Base ===\n");
        int index = 0;

        for (const auto &entry : Q)
        {
            const std::string &key = entry.first;
            const std::set<std::string> &value = entry.second;

            Serial.printf("[%d] Key: %s, Value: {", index++, key.c_str());
            for (const auto &id : value)
            {
                Serial.printf("%s ", id.c_str());
            }
            Serial.printf("}\n");
        }

        Serial.printf("======================\n\n");
    }
};
