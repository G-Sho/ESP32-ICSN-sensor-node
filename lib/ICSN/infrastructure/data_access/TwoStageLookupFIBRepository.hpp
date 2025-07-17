#pragma once

#include "data_access/IFIBRepository.hpp"
#include "config/Config.hpp"
#include <algorithm>
#include <unordered_map>
#include <set>
#include <string>
#include <list>

template <typename T>
bool chmax(T &a, const T &b)
{
    if (a < b)
    {
        a = b;
        return 1;
    }
    return 0;
}

class TwoStageLookupFIBRepository : public IFIBRepository
{
private:
    // FIBエントリの構造体
    struct FIBEntry
    {
    public:
        bool isVir;
        int maximumDepth;
        std::set<std::string> nodeId;
        FIBEntry() = default;
        FIBEntry(bool flg, int num, std::set<std::string> Id)
        {
            isVir = flg;
            maximumDepth = num;
            nodeId = Id;
        }
        bool getIsVir() const { return isVir; };
        int getMaximumDepth() const { return maximumDepth; };
        const std::set<std::string> &getNodeId() const { return nodeId; };
        void setIsVir(bool flg) { isVir = flg; };
        void setMaximumDepth(int m) { maximumDepth = m; };
        void insertNodeId(const std::string &id) { nodeId.insert(id); };
    };

    // std::map<std::string, FIBEntry> fib;
    std::list<std::pair<std::string, FIBEntry>> Q;
    std::unordered_map<std::string, std::list<std::pair<std::string, FIBEntry>>::iterator> iter;

    std::string extractPrefix(const std::string &name, int pfx)
    {
        int cnt = 0;
        std::string res = name;

        for (int i = 0; i < res.length(); i++)
        {
            if (res[i] == '/')
            {
                cnt++;
                if (cnt > pfx)
                {
                    return res.substr(0, i);
                }
            }
        }

        return res;
    }

    // 新しいFIB Tableを構築する関数
    void SaveFIB(const std::string &contentName, const std::set<std::string> &nodeId, int m)
    {
        if (systemConfig.maxVirtualDepth >= m && iter.count(contentName))
        {
            iter[contentName]->second.setIsVir(false);
            for (const auto &x : nodeId)
                iter[contentName]->second.insertNodeId(x);
        }
        else if (systemConfig.maxVirtualDepth >= m && !iter.count(contentName))
        {
            Q.push_front({contentName, FIBEntry(false, m, nodeId)});
            iter[contentName] = Q.begin();
        }
        else
        {
            std::string str = extractPrefix(contentName, systemConfig.maxVirtualDepth);
            if (iter.count(str))
            {
                chmax(iter[str]->second.maximumDepth, m);
            }
            else
            {
                Q.push_front({str, FIBEntry(true, m, {})});
                iter[str] = Q.begin();
            }

            if (iter.count(contentName))
            {
                for (const auto &x : nodeId)
                    iter[contentName]->second.insertNodeId(x);
            }
            else
            {
                Q.push_front({contentName, FIBEntry(false, m, nodeId)});
                iter[contentName] = Q.begin();
            }
        }
    };

    // FIBを検索する関数
    FIBEntry *LookupFIB(const std::string &name, const int &pfx)
    {
        std::string prefix = extractPrefix(name, pfx);
        if (iter.count(prefix))
        {
            return &iter[prefix]->second;
        }
        return nullptr;
    };

    // エントリが仮想かどうかを判定する関数
    bool IsVirtual(FIBEntry *entry)
    {
        return entry && entry->isVir;
    };

    // プレフィックスの最大深度を取得する関数
    int MD(FIBEntry *entry)
    {
        return entry ? entry->maximumDepth : 0;
    };

    // FIBのLongest Prefix Match (LPM) 処理を行う関数
    FIBEntry *FIB_LPM_LOOKUP(const std::string &name, int n, int M)
    {
        FIBEntry *FIB_entry = nullptr;
        FIBEntry *FIB_entry_1s = nullptr;
        int start_pfx;
        // FIB_LPM_STAGE_1
        bool stage2Flg = false;
        for (int pfx = std::min(n, M); pfx >= 1; --pfx)
        {
            FIB_entry = LookupFIB(name, pfx);
            if (FIB_entry)
            {
                if (MD(FIB_entry) > M && n > M)
                {
                    start_pfx = std::min(n, MD(FIB_entry));
                    FIB_entry_1s = FIB_entry;
                    stage2Flg = true;
                    break;
                }
                else if (!IsVirtual(FIB_entry))
                {
                    return FIB_entry; // 仮想エントリでなければそのまま返す
                }
            }
        }
        // FIB_LPM_STAGE_2
        if (stage2Flg)
        {
            for (int pfx = start_pfx; pfx >= 1; --pfx)
            {
                if (pfx == M)
                {
                    FIB_entry = FIB_entry_1s;
                    if (!IsVirtual(FIB_entry))
                    {
                        return FIB_entry;
                    }
                }
                FIB_entry = LookupFIB(name, pfx);
                if (FIB_entry && !IsVirtual(FIB_entry))
                {
                    return FIB_entry;
                }
            }
        }
        return nullptr; // FIB_NOT_FOUND
    }

public:
    void save(const FIBPair &fibPair) override;
    void remove(const ContentName &contentName) override;
    bool find(const ContentName &contentName) override;
    DestinationId get(const ContentName &contentName) override;
};
