#pragma once
#include <stdint.h>
#include <esp_timer.h>

#define MAX_MEASUREMENTS 200

struct SensorMeasurement {
    uint32_t interest_rx_us = 0;   // Interest受信
    uint32_t ota_start_us   = 0;   // OTA検証開始
    uint32_t ota_end_us     = 0;   // OTA検証終了
    uint32_t fib_lookup_us  = 0;   // FIB検索完了（時刻）
    uint32_t sensor_tx_us   = 0;   // 次ホップ送信完了
    uint32_t data_rx_us     = 0;   // Data受信（中継ノード用）
};

class SensorPerformanceBuffer {
private:
    SensorMeasurement buffer[MAX_MEASUREMENTS];
    uint16_t index = 0;
    uint16_t pending_seq = 0xFFFF;

public:
    inline void recordInterestRx() {
        if (index >= MAX_MEASUREMENTS) return;
        buffer[index] = SensorMeasurement{};   // フィールドをゼロクリア
        buffer[index].interest_rx_us = (uint32_t)esp_timer_get_time();
        pending_seq = index;
    }

    inline void recordOtaStart() {
        if (pending_seq >= MAX_MEASUREMENTS) return;
        buffer[pending_seq].ota_start_us = (uint32_t)esp_timer_get_time();
    }

    inline void recordOtaEnd() {
        if (pending_seq >= MAX_MEASUREMENTS) return;
        buffer[pending_seq].ota_end_us = (uint32_t)esp_timer_get_time();
    }

    inline void recordFibLookup() {
        if (pending_seq >= MAX_MEASUREMENTS) return;
        buffer[pending_seq].fib_lookup_us = (uint32_t)esp_timer_get_time();
    }

    inline void recordSensorTx() {
        if (pending_seq >= MAX_MEASUREMENTS) return;
        buffer[pending_seq].sensor_tx_us = (uint32_t)esp_timer_get_time();
        index++;
        pending_seq = 0xFFFF;
    }

    inline void recordDataRx() {
        if (pending_seq >= MAX_MEASUREMENTS) return;
        buffer[pending_seq].data_rx_us = (uint32_t)esp_timer_get_time();
    }

    uint16_t getCount() const { return index; }
    const SensorMeasurement& getEntry(uint16_t i) const { return buffer[i]; }
    void reset() { index = 0; pending_seq = 0xFFFF; }
};

extern SensorPerformanceBuffer g_sensor_perf;
