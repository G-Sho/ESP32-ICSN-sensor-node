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
    void save(const CSPair &csPair) override;
    void remove(const ContentName &contentName) override;
    bool find(const ContentName &contentName) override;
    Content get(const ContentName &contentName) override;

    void clear()
    {
        cache.clear();
    }

    void printCache() const
    {
        CLI_PRINTLN("=== Content Store ===");
        cache.printCache();
    }
};
