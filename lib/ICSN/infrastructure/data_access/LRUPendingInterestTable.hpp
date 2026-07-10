#pragma once

#include "../../BuildProfile.hpp"
#include "../../config/Config.hpp"
#include "../../interface/data_access/IPendingInterestTable.hpp"
#include "FixedNodeIdSet.hpp"
#include "FixedSizeLRUCache.hpp"
#include <string>

class LRUPendingInterestTable : public IPendingInterestTable {
private:
  using RequesterSet =
      FixedNodeIdSet<BuildCapacity::PIT_REQUESTERS_PER_ENTRY, BuildCapacity::NODE_ID_MAX_CHARS>;
  FixedSizeLRUCache<RequesterSet, BuildCapacity::PIT_ENTRIES> cache;

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

  void printUsageStats() const {
    CLI_PRINTF("[PIT] entries=%u/%u, requesters_per_entry=%u\n",
               static_cast<unsigned int>(cache.size()),
               static_cast<unsigned int>(BuildCapacity::PIT_ENTRIES),
               static_cast<unsigned int>(BuildCapacity::PIT_REQUESTERS_PER_ENTRY));
  }
};
