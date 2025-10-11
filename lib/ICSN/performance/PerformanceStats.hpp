#pragma once

#include <Arduino.h>
#include <esp_timer.h>
#include <cstdint>
#include <climits>

class PerformanceStats {
private:
    int64_t total_time;
    int64_t min_time;
    int64_t max_time;
    uint32_t count;

public:
    PerformanceStats();

    void addMeasurement(int64_t elapsed_us);
    void printStats(const char* name) const;
    void reset();

    // ゲッター関数
    double getAverageUs() const;
    int64_t getMinUs() const;
    int64_t getMaxUs() const;
    uint32_t getCount() const;
};

// RAII時間測定クラス
class ScopedTimer {
private:
    int64_t start_time;
    const char* label;
    PerformanceStats* stats;

public:
    ScopedTimer(const char* label, PerformanceStats* stats);
    ~ScopedTimer();
};

// 測定マクロ
#ifdef PERFORMANCE_MEASURE
    #define MEASURE_SCOPE(label, stats) ScopedTimer timer(label, stats)
    #define MEASURE_START(var) int64_t var = esp_timer_get_time()
    #define MEASURE_END(var, stats) stats.addMeasurement(esp_timer_get_time() - var)
#else
    #define MEASURE_SCOPE(label, stats)
    #define MEASURE_START(var)
    #define MEASURE_END(var, stats)
#endif
