#pragma once

#include "../entity/routing_table/FIBPair.hpp"
#include "../entity/message/ContentName.hpp"
#include "../entity/message/DestinationId.hpp"

class IForwardingInformationBase
{
public:
    /// @brief real entry を保存する（isVirtual=false）
    virtual void save(const FIBPair &fibPair) = 0;
    /// @brief virtual entry を保存または最大深度を更新する（isVirtual=true）
    /// @param prefix       保存するプレフィックス
    /// @param maximumDepth このプレフィックス配下の最大エントリ深度
    virtual void saveVirtualEntry(const ContentName &prefix, int maximumDepth) = 0;
    virtual void remove(const ContentName &contentName) = 0;
    virtual bool find(const ContentName &contentName) = 0;
    virtual DestinationId get(const ContentName &contentName) = 0;
    virtual void printCache() const = 0;
    virtual void setActiveSize(size_t size) = 0;
    virtual size_t getActiveSize() const = 0;

    virtual ~IForwardingInformationBase() = default;
};
