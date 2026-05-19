#pragma once

#include <string>

class IManagementBoundary
{
public:
    virtual void initFIBEntry(const std::string &contentName, const std::string &nextHopMac) = 0;
    virtual void printFIB() const = 0;
    virtual void clearCSCache() = 0;
    virtual void clearPITCache() = 0;

    virtual ~IManagementBoundary() = default;
};
