#pragma once

#include "../../BuildProfile.hpp"
#include "../../config/Config.hpp"
#include "../../interface/data_access/IContentStore.hpp"
#include "FixedSizeLRUCache.hpp"
#include <cstddef>
#include <string>

class LRUContentStore : public IContentStore {
private:
  struct PayloadUsageStats {
    size_t usedSlots;
    size_t heapBytes;
    size_t psramBytes;

    PayloadUsageStats() : usedSlots(0), heapBytes(0), psramBytes(0) {}
  };

  struct PayloadSlot {
    char* buffer;
    size_t length;
    bool inUse;
    bool inPsram;

    PayloadSlot() : buffer(nullptr), length(0), inUse(false), inPsram(false) {}
  };

  FixedSizeLRUCache<int, BuildCapacity::CS_ENTRIES> cache;
  PayloadSlot payloadSlots[BuildCapacity::CS_ENTRIES];

  int findFreePayloadSlot() const;
  void releasePayloadSlot(int slotIndex);
  bool allocatePayloadBuffer(size_t bytes, char*& outBuffer, bool& outInPsram);
  bool storePayload(const std::string& content, int& outSlotIndex);
  PayloadUsageStats collectPayloadUsageStats() const;

public:
  LRUContentStore() = default;
  ~LRUContentStore() override;

  void save(const CSPair& csPair) override;
  void remove(const ContentName& contentName) override;
  bool find(const ContentName& contentName) override;
  Content get(const ContentName& contentName) override;
  void clear() override;

  void printCache() const {
    CLI_PRINTLN("=== Content Store ===");
    cache.printCache();
  }

  void printPayloadStats() const;
};
