#pragma once

#include "../../BuildProfile.hpp"
#include "../../config/Config.hpp"
#include "../../interface/data_access/IForwardingInformationBase.hpp"
#include "FixedNodeIdSet.hpp"
#include "FixedSizeLRUCache.hpp"
#include <algorithm>
#include <string>

class LRUForwardingInformationBase : public IForwardingInformationBase {
private:
  using NextHopSet =
      FixedNodeIdSet<BuildCapacity::FIB_NEXT_HOPS_PER_ENTRY, BuildCapacity::NODE_ID_MAX_CHARS>;

  // TwoStage用のFIBエントリ構造体
  struct FIBEntry {
    bool isVirtual;
    int maximumDepth;
    NextHopSet nodeIds;

    FIBEntry() : isVirtual(false), maximumDepth(0) {}
    FIBEntry(bool isVir, int maxDepth, const NextHopSet& nodes)
        : isVirtual(isVir), maximumDepth(maxDepth), nodeIds(nodes) {}
  };

  FixedSizeLRUCache<FIBEntry, BuildCapacity::FIB_ENTRIES> cache;

  // TwoStageアルゴリズム用ヘルパー関数
  std::string extractPrefix(const std::string& name, int prefixDepth) const;
  bool lookupEntry(const std::string& name, int prefixDepth, FIBEntry& outEntry);
  bool fibLpmLookup(const std::string& name, int nameDepth, int maxVirtualDepth,
                    FIBEntry& outEntry);

  template <typename T> bool chmax(T& a, const T& b) {
    if (a < b) {
      a = b;
      return true;
    }
    return false;
  }

public:
  LRUForwardingInformationBase() = default;

  void save(const FIBPair& fibPair) override;
  void saveVirtualEntry(const ContentName& prefix, int maximumDepth) override;
  void remove(const ContentName& contentName) override;
  bool find(const ContentName& contentName) override;
  DestinationId get(const ContentName& contentName) override;

  void printCache() const override {
    CLI_PRINTLN("=== Forwarding Information Base (TwoStage + LRU) ===");
    cache.printCache();
    CLI_PRINTLN("======================");
  }

  void printUsageStats() const {
    CLI_PRINTF("[FIB] entries=%u/%u, next_hops_per_entry=%u\n",
               static_cast<unsigned int>(cache.size()),
               static_cast<unsigned int>(BuildCapacity::FIB_ENTRIES),
               static_cast<unsigned int>(BuildCapacity::FIB_NEXT_HOPS_PER_ENTRY));
  }
};
