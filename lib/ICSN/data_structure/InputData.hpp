#pragma once

#include "message/SignalCode.hpp"
#include <string>
#include <set>
#include <cstdint>

struct InputData
{
    std::string senderId;
    std::set<std::string> destId;
    std::string signalCode;
    int hopCount;
    std::string contentName;
    std::string content;
    uint32_t time;

    InputData(const std::string& senderId,
              const std::set<std::string>& destId,
              const std::string& signalCode,
              int hopCount,
              const std::string& contentName,
              const std::string& content,
              uint32_t time)
        : senderId(senderId), destId(destId), signalCode(signalCode),
          hopCount(hopCount), contentName(contentName), content(content), time(time) {}
};