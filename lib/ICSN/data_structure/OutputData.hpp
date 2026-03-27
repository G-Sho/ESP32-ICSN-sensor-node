#pragma once

#include "message/SignalCode.hpp"
#include <string>
#include <set>
#include <cstdint>

constexpr const char* VALUE_NA = "N/A";             // 使用しない、該当しない値
constexpr const char* VALUE_UNKNOWN = "UNKNOWN";    // 未知・未確認の情報

struct OutputData
{
    std::string senderId;
    std::set<std::string> destId;
    std::string signalCode;
    int hopCount;
    std::string contentName;
    std::string content;

    OutputData(const std::string& senderId,
              const std::set<std::string>& destId,
              const std::string& signalCode,
              int hopCount,
              const std::string& contentName,
              const std::string& content)
        : senderId(senderId), destId(destId), signalCode(signalCode),
          hopCount(hopCount), contentName(contentName), content(content) {}

    OutputData() = default;
};

inline OutputData makeOutput(
    const std::string& dest,
    const std::set<std::string>& destId,
    const std::string& signal,
    int hopCount,
    const std::string& contentName,
    const std::string& contentValue)
{
    return OutputData(dest, destId, signal, hopCount, contentName, contentValue);
};

inline OutputData makeOutput()
{
    return makeOutput(
        VALUE_NA,
        {VALUE_NA},
        toString(SignalCode::INVALID),
        0,
        VALUE_NA,
        VALUE_NA
    );
};