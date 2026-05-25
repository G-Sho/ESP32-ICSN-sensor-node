#pragma once

#include <string>

/// @brief RIB (Routing Information Base) のデータアクセスインターフェース
/// @details Control plane のルート追加・削除を担う。
///          実装は FIB への反映（virtual entry 生成を含む）も行う。
class IRIB
{
public:
    /// @brief ルートを追加または更新する
    /// @param contentName コンテンツ名（フルパス）
    /// @param nextHopId   次ホップのノードID（MACアドレス文字列等）
    virtual void addRoute(const std::string &contentName, const std::string &nextHopId) = 0;

    /// @brief ルートを削除する
    /// @param contentName コンテンツ名（フルパス）
    virtual void removeRoute(const std::string &contentName) = 0;

    virtual ~IRIB() = default;
};
