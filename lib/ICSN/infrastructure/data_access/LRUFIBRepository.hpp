#pragma once

#include "../../interface/data_access/IFIBRepository.hpp"
#include "FixedSizeLRUCache.hpp"
#include "../../config/Config.hpp"
#include <set>
#include <string>
#include <algorithm>
#include <Arduino.h>

class LRUFIBRepository : public IFIBRepository
{
private:
    // TwoStage用のFIBエントリ構造体
    struct FIBEntry
    {
        bool isVirtual;
        int maximumDepth;
        std::set<std::string> nodeIds;
        
        FIBEntry() : isVirtual(false), maximumDepth(0) {}
        FIBEntry(bool isVir, int maxDepth, const std::set<std::string>& nodes)
            : isVirtual(isVir), maximumDepth(maxDepth), nodeIds(nodes) {}
    };

    FixedSizeLRUCache<FIBEntry, MAX_FIB_TABLE_SIZE> cache;

    // TwoStageアルゴリズム用ヘルパー関数
    std::string extractPrefix(const std::string &name, int prefixDepth) const;
    bool lookupEntry(const std::string &name, int prefixDepth, FIBEntry& outEntry);
    bool fibLpmLookup(const std::string &name, int nameDepth, int maxVirtualDepth, FIBEntry& outEntry);
    void saveFibEntry(const std::string &contentName, const std::set<std::string> &nodeIds, int depth);
    
    template<typename T>
    bool chmax(T &a, const T &b) {
        if (a < b) { a = b; return true; }
        return false;
    }

public:
    void save(const FIBPair &fibPair) override;
    void remove(const ContentName &contentName) override;
    bool find(const ContentName &contentName) override;
    DestinationId get(const ContentName &contentName) override;

    void printCache() const
    {
        Serial.printf("=== Forwarding Information Base (TwoStage + LRU) ===\n");
        cache.printCache();
        Serial.printf("======================\n");
    }
};
