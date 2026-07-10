#include "LRUContentStore.hpp"
#include "../../config/Config.hpp"
#include <cstdlib>
#include <cstring>

#if defined(ESP_PLATFORM)
#include <esp_heap_caps.h>
#endif

int LRUContentStore::findFreePayloadSlot() const {
  for (int i = 0; i < static_cast<int>(BuildCapacity::CS_ENTRIES); ++i) {
    if (!payloadSlots[i].inUse) {
      return i;
    }
  }
  return -1;
}

void LRUContentStore::releasePayloadSlot(int slotIndex) {
  if (slotIndex < 0 || slotIndex >= static_cast<int>(BuildCapacity::CS_ENTRIES)) {
    return;
  }

  PayloadSlot& slot = payloadSlots[slotIndex];
  if (slot.buffer != nullptr) {
    free(slot.buffer);
  }

  slot.buffer = nullptr;
  slot.length = 0;
  slot.inUse = false;
  slot.inPsram = false;
}

bool LRUContentStore::allocatePayloadBuffer(size_t bytes, char*& outBuffer, bool& outInPsram) {
  outBuffer = nullptr;
  outInPsram = false;

#if defined(ESP_PLATFORM) && defined(BOARD_HAS_PSRAM)
  if (BuildMemoryPolicy::CS_PAYLOAD_PSRAM_PREFERRED) {
    outBuffer = static_cast<char*>(heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (outBuffer != nullptr) {
      outInPsram = true;
      return true;
    }
  }
#endif

  outBuffer = static_cast<char*>(malloc(bytes));
  return outBuffer != nullptr;
}

bool LRUContentStore::storePayload(const std::string& content, int& outSlotIndex) {
  outSlotIndex = -1;

  int slotIndex = findFreePayloadSlot();
  if (slotIndex < 0) {
    return false;
  }

  const size_t bytes = content.size() + 1;
  char* buffer = nullptr;
  bool inPsram = false;
  if (!allocatePayloadBuffer(bytes, buffer, inPsram)) {
    return false;
  }

  memcpy(buffer, content.c_str(), bytes);

  PayloadSlot& slot = payloadSlots[slotIndex];
  slot.buffer = buffer;
  slot.length = content.size();
  slot.inUse = true;
  slot.inPsram = inPsram;

  outSlotIndex = slotIndex;
  return true;
}

LRUContentStore::PayloadUsageStats LRUContentStore::collectPayloadUsageStats() const {
  PayloadUsageStats stats;
  for (int i = 0; i < static_cast<int>(BuildCapacity::CS_ENTRIES); ++i) {
    const PayloadSlot& slot = payloadSlots[i];
    if (!slot.inUse) {
      continue;
    }

    stats.usedSlots++;
    if (slot.inPsram) {
      stats.psramBytes += slot.length;
    } else {
      stats.heapBytes += slot.length;
    }
  }
  return stats;
}

LRUContentStore::~LRUContentStore() {
  clear();
}

void LRUContentStore::save(const CSPair& csPair) {
  const std::string& name = csPair.getContentName().getValue();
  const std::string& content = csPair.getContent().getValue();

  int oldSlotIndex = -1;
  const bool hasExisting = cache.get(name, oldSlotIndex);

  int newSlotIndex = -1;
  if (!storePayload(content, newSlotIndex)) {
    LOG_WARNF("[CS] Failed to allocate payload for key=%s\n", name.c_str());
    return;
  }

  if (!cache.put(name, newSlotIndex)) {
    releasePayloadSlot(newSlotIndex);
    LOG_WARNF("[CS] Capacity full, cannot cache key=%s\n", name.c_str());
    return;
  }

  if (hasExisting) {
    releasePayloadSlot(oldSlotIndex);
  }

  LOG_DEBUGF("[CS] Saved key=%s, slot=%d, payload=%s\n", name.c_str(), newSlotIndex,
             payloadSlots[newSlotIndex].inPsram ? "psram" : "heap");
}

void LRUContentStore::remove(const ContentName& contentName) {
  const std::string& name = contentName.getValue();

  int slotIndex = -1;
  const bool found = cache.get(name, slotIndex);
  cache.remove(name);

  if (found) {
    releasePayloadSlot(slotIndex);
  }
}

bool LRUContentStore::find(const ContentName& contentName) {
  return cache.contains(contentName.getValue());
}

Content LRUContentStore::get(const ContentName& contentName) {
  const std::string& name = contentName.getValue();
  int slotIndex = -1;

  if (!cache.get(name, slotIndex)) {
    return Content::Null();
  }

  if (slotIndex < 0 || slotIndex >= static_cast<int>(BuildCapacity::CS_ENTRIES)) {
    return Content::Null();
  }

  const PayloadSlot& slot = payloadSlots[slotIndex];
  if (!slot.inUse || slot.buffer == nullptr) {
    return Content::Null();
  }

  return Content(std::string(slot.buffer, slot.length));
}

void LRUContentStore::clear() {
  cache.clear();
  for (int i = 0; i < static_cast<int>(BuildCapacity::CS_ENTRIES); ++i) {
    releasePayloadSlot(i);
  }
}

void LRUContentStore::printPayloadStats() const {
  const PayloadUsageStats stats = collectPayloadUsageStats();
  CLI_PRINTF("[CS] payload slots=%u/%u, heap=%uB, psram=%uB\n",
             static_cast<unsigned int>(stats.usedSlots),
             static_cast<unsigned int>(BuildCapacity::CS_ENTRIES),
             static_cast<unsigned int>(stats.heapBytes),
             static_cast<unsigned int>(stats.psramBytes));
}
