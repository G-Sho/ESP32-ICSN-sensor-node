#pragma once

#include <cstdint>
#include <esp_timer.h>

struct InterestPacketTimingEntry
{
    uint32_t interest_rx_us = 0;
    uint32_t security_check_start_us = 0;
    uint32_t security_check_end_us = 0;
    uint32_t fib_lookup_us = 0;
    uint32_t forward_tx_us = 0;
};

class InterestPacketTimingBuffer
{
private:
    static constexpr uint16_t MAX_MEASUREMENTS = 200;

    InterestPacketTimingEntry buffer[MAX_MEASUREMENTS];
    uint16_t index = 0;
    uint16_t pending_seq = 0xFFFF;

public:
    void recordInterestRx();
    void recordSecurityCheckStart();
    void recordSecurityCheckEnd();
    void recordFibLookup();
    void recordForwardTx();

    uint16_t getCount() const;
    const InterestPacketTimingEntry &getEntry(uint16_t i) const;
    void reset();
};
