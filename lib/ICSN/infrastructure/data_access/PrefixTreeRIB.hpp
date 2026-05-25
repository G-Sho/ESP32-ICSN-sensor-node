#pragma once

#include "../../interface/data_access/IRIB.hpp"
#include "../../interface/data_access/IForwardingInformationBase.hpp"
#include "../../entity/routing_table/RIBNode.hpp"
#include "../../config/Config.hpp"
#include <unordered_map>
#include <string>

/// @brief プレフィックスツリーに基づく RIB 実装
/// @details コンテンツ名をキーとするハッシュマップでルーティング情報を管理し、
///          FIB への real/virtual エントリ書き込みを担う Control plane 実装。
class PrefixTreeRIB : public IRIB
{
private:
    IForwardingInformationBase &fibRepository;
    std::unordered_map<std::string, RIBNode> tree;

    /// @brief コンテンツ名から指定深さのプレフィックスを抽出する
    static std::string extractPrefix(const std::string &name, int prefixDepth);

public:
    /// @param fib 書き込み先の FIB インスタンス
    explicit PrefixTreeRIB(IForwardingInformationBase &fib);

    /// @brief ルートを追加または更新し、FIB へ反映する
    void addRoute(const std::string &contentName, const std::string &nextHopId) override;

    /// @brief ルートを削除し、対応する FIB エントリを整理する
    void removeRoute(const std::string &contentName) override;
};
