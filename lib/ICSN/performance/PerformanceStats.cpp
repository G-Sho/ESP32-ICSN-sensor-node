#include "PerformanceStats.hpp"

PerformanceStats::PerformanceStats() {
    reset();
}

void PerformanceStats::addMeasurement(int64_t elapsed_us) {
    total_time += elapsed_us;
    count++;

    if (elapsed_us < min_time) {
        min_time = elapsed_us;
    }

    if (elapsed_us > max_time) {
        max_time = elapsed_us;
    }
}

void PerformanceStats::printStats(const char* name) const {
    if (count == 0) {
        Serial.printf("[PERF] %s: No measurements\n", name);
        return;
    }

    double avg_us = static_cast<double>(total_time) / count;

    Serial.printf("[PERF] %s Stats:\n", name);
    Serial.printf("  Count: %u\n", count);
    Serial.printf("  Avg:   %.2f μs\n", avg_us);
    Serial.printf("  Min:   %lld μs\n", min_time);
    Serial.printf("  Max:   %lld μs\n", max_time);
    Serial.printf("  Total: %lld μs\n", total_time);
    Serial.println("---");
}

void PerformanceStats::reset() {
    total_time = 0;
    min_time = LLONG_MAX;
    max_time = 0;
    count = 0;
}

double PerformanceStats::getAverageUs() const {
    if (count == 0) return 0.0;
    return static_cast<double>(total_time) / count;
}

int64_t PerformanceStats::getMinUs() const {
    return (count == 0) ? 0 : min_time;
}

int64_t PerformanceStats::getMaxUs() const {
    return max_time;
}

uint32_t PerformanceStats::getCount() const {
    return count;
}

// ScopedTimer implementation
ScopedTimer::ScopedTimer(const char* label, PerformanceStats* stats)
    : label(label), stats(stats) {
    start_time = esp_timer_get_time();
}

ScopedTimer::~ScopedTimer() {
    int64_t elapsed = esp_timer_get_time() - start_time;
    if (stats) {
        stats->addMeasurement(elapsed);
    }
}
