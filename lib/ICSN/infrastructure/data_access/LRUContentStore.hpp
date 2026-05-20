#pragma once

#include "../../interface/data_access/IContentStore.hpp"
#include "FixedSizeLRUCache.hpp"
#include "../../config/Config.hpp"
#include "../../BuildProfile.hpp"
#include <string>

class LRUContentStore : public IContentStore
{
private:
    FixedSizeLRUCache<std::string, MAX_CS_TABLE_SIZE> cache;

public:
    /// @brief コンストラクタ（デフォルト activeSize は MAX_CS_TABLE_SIZE）
    LRUContentStore(size_t activeSize = MAX_CS_TABLE_SIZE) 
        : cache(activeSize) {}

    void save(const CSPair &csPair) override;
    void remove(const ContentName &contentName) override;
    bool find(const ContentName &contentName) override;
    Content get(const ContentName &contentName) override;

    void clear() override
    {
        cache.clear();
    }

    void printCache() const
    {
        CLI_PRINTLN("=== Content Store ===");
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
