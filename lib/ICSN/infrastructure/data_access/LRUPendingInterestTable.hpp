#pragma once

#include "../../BuildProfile.hpp"
#include "../../config/Config.hpp"
#include "../../interface/data_access/IPendingInterestTable.hpp"
#include "FixedSizeLRUCache.hpp"
#include <set>
#include <string>

class LRUPendingInterestTable : public IPendingInterestTable {
private:
  FixedSizeLRUCache<std::set<std::string>, BuildCapacity::PIT_ENTRIES> cache;

public:
  LRUPendingInterestTable() = default;

  void save(const PITPair& pitPair) override;
  void remove(const ContentName& contentName) override;
  bool find(const ContentName& contentName) override;
  DestinationId get(const ContentName& contentName) override;

  void clear() override {
    cache.clear();
  }

  void printCache() const {
    CLI_PRINTLN("=== Pending Interest Table ===");
    cache.printCache();
  }
};
