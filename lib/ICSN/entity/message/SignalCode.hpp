#pragma once

#include <string>

enum class SignalCode {
    INTEREST,
    DATA,
    INVALID
};

inline std::string toString(SignalCode code) {
    switch (code) {
        case SignalCode::INTEREST: return "INTEREST";
        case SignalCode::DATA:     return "DATA";
        case SignalCode::INVALID:  return "INVALID";
        default:                   return "UNKNOWN";
    }
};

inline SignalCode fromString(const std::string& s) {
    if (s == "INTEREST") return SignalCode::INTEREST;
    if (s == "DATA")     return SignalCode::DATA;
    return SignalCode::INVALID;
};