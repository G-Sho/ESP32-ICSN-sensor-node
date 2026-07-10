#pragma once

#include "../../BuildProfile.hpp"
#include "../../config/Config.hpp"
#include "../../interface/data_access/IContentStore.hpp"
#include "FixedSizeLRUCache.hpp"
#include <string>

class LRUContentStore : public IContentStore {
private:
  FixedSizeLRUCache<std::string, BuildCapacity::CS_ENTRIES> cache;

public:
  LRUContentStore() = default;

  void save(const CSPair& csPair) override;
  void remove(const ContentName& contentName) override;
  bool find(const ContentName& contentName) override;
  Content get(const ContentName& contentName) override;

  void clear() override {
    cache.clear();
  }

  void printCache() const {
    CLI_PRINTLN("=== Content Store ===");
    cache.printCache();
  }
};
