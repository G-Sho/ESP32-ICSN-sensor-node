#include "MemoryStats.hpp"

#include "BuildProfile.hpp"

#include <esp_heap_caps.h>

namespace {

HeapStats collectStatsByCaps(uint32_t caps) {
  const size_t totalBytes = heap_caps_get_total_size(caps);
  const bool available = (totalBytes > 0U);

  if (!available) {
    return {0U, 0U, 0U, false};
  }

  return {
      totalBytes,
      heap_caps_get_free_size(caps),
      heap_caps_get_minimum_free_size(caps),
      true,
  };
}

void printBytesAndKiB(const size_t bytes) {
  const size_t kib = bytes / 1024U;
  CLI_PRINTF("%uB (%uKiB)", static_cast<unsigned int>(bytes), static_cast<unsigned int>(kib));
}

void printRegionLine(const char *label, const HeapStats &stats) {
  CLI_PRINTF("[MEM] %s: ", label);

  if (!stats.available) {
    CLI_PRINTLN("not available");
    return;
  }

  CLI_PRINTF("total=");
  printBytesAndKiB(stats.totalBytes);
  CLI_PRINTF(", free=");
  printBytesAndKiB(stats.freeBytes);
  CLI_PRINTF(", min=");
  printBytesAndKiB(stats.minFreeBytes);
  CLI_PRINTLN();
}

}  // namespace

MemorySnapshot collectMemorySnapshot() {
  MemorySnapshot snapshot{};
  snapshot.internal = collectStatsByCaps(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  snapshot.psram = collectStatsByCaps(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  return snapshot;
}

void printMemorySnapshot(const MemorySnapshot &snapshot, const char *header) {
  CLI_PRINTF("[MEM] %s\n", (header != nullptr) ? header : "snapshot");
  printRegionLine("internal", snapshot.internal);
  printRegionLine("psram", snapshot.psram);
}
