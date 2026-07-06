#pragma once

#include <Arduino.h>

struct HeapStats {
  size_t totalBytes;
  size_t freeBytes;
  size_t minFreeBytes;
  bool available;
};

struct MemorySnapshot {
  HeapStats internal;
  HeapStats psram;
};

MemorySnapshot collectMemorySnapshot();
void printMemorySnapshot(const MemorySnapshot &snapshot, const char *header);
