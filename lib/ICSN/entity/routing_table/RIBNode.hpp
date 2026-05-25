#pragma once

#include <set>
#include <string>

/// @brief RIB (Routing Information Base) のプレフィックスツリー上の1ノード
struct RIBNode
{
    std::set<std::string> nextHopIds; ///< このコンテンツ名に対する次ホップID集合
    bool isReal;                      ///< 実際に学習したルートか（false = 中間ノードのみ）

    RIBNode() : isReal(false) {}
    RIBNode(const std::set<std::string> &ids, bool real)
        : nextHopIds(ids), isReal(real) {}
};
