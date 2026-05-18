#include "InterestPacketTimingBuffer.hpp"

void InterestPacketTimingBuffer::recordInterestRx()
{
    if (index >= MAX_MEASUREMENTS)
    {
        return;
    }

    buffer[index] = InterestPacketTimingEntry{};
    buffer[index].interest_rx_us = static_cast<uint32_t>(esp_timer_get_time());
    pending_seq = index;
}

void InterestPacketTimingBuffer::recordSecurityCheckStart()
{
    if (pending_seq >= MAX_MEASUREMENTS)
    {
        return;
    }

    buffer[pending_seq].security_check_start_us = static_cast<uint32_t>(esp_timer_get_time());
}

void InterestPacketTimingBuffer::recordSecurityCheckEnd()
{
    if (pending_seq >= MAX_MEASUREMENTS)
    {
        return;
    }

    buffer[pending_seq].security_check_end_us = static_cast<uint32_t>(esp_timer_get_time());
}

void InterestPacketTimingBuffer::recordFibLookup()
{
    if (pending_seq >= MAX_MEASUREMENTS)
    {
        return;
    }

    buffer[pending_seq].fib_lookup_us = static_cast<uint32_t>(esp_timer_get_time());
}

void InterestPacketTimingBuffer::recordForwardTx()
{
    if (pending_seq >= MAX_MEASUREMENTS)
    {
        return;
    }

    buffer[pending_seq].forward_tx_us = static_cast<uint32_t>(esp_timer_get_time());
    index++;
    pending_seq = 0xFFFF;
}

uint16_t InterestPacketTimingBuffer::getCount() const
{
    return index;
}

const InterestPacketTimingEntry &InterestPacketTimingBuffer::getEntry(uint16_t i) const
{
    return buffer[i];
}

void InterestPacketTimingBuffer::reset()
{
    index = 0;
    pending_seq = 0xFFFF;
}
