#pragma once

#include "../../interface/data_access/IPendingInterestTable.hpp"
#include "FixedSizeLRUCache.hpp"
#include "../../config/Config.hpp"
#include "../../BuildProfile.hpp"
#include <set>
#include <string>

class LRUPendingInterestTable : public IPendingInterestTable
{
private:
    FixedSizeLRUCache<std::set<std::string>, MAX_PIT_TABLE_SIZE> cache;

public:
    /// @brief コンストラクタ（デフォルト activeSize は MAX_PIT_TABLE_SIZE）
    LRUPendingInterestTable(size_t activeSize = MAX_PIT_TABLE_SIZE) 
        : cache(activeSize) {}

    void save(const PITPair &pitPair) override;
    void remove(const ContentName &contentName) override;
    bool find(const ContentName &contentName) override;
    DestinationId get(const ContentName &contentName) override;

    void clear() override
    {
        cache.clear();
    }

    void printCache() const
    {
        CLI_PRINTLN("=== Pending Interest Table ===");
        cache.printCache();
    }
    
    void setActiveSize(size_t size) override
    {
        cache.setActiveSize(size);
    }
    
    size_t getActiveSize() const override
    {
        return cache.getActiveSize();
    }
};
