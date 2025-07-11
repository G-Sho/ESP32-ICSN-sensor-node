#ifndef INCLUDED_TWO_STAGE_LOOKUP_FIB_REPOSITORY_hpp_
#define INCLUDED_TWO_STAGE_LOOKUP_FIB_REPOSITORY_hpp_

#include "interface/FIBRepositoryInterface.hpp"
#include <map>       //std::map
#include <algorithm> // std::min
#include <unordered_map>

#define THRESHOLD 5 // 論文中の M
#define MAX_FIB_TABLE_SIZE 20

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

class TwoStageLookupFIBRepository : public FIBRepositoryInterface
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
        if (THRESHOLD >= m && iter.count(contentName))
        {
            iter[contentName]->second.setIsVir(false);
            for (const auto &x : nodeId)
                iter[contentName]->second.insertNodeId(x);
        }
        else if (THRESHOLD >= m && !iter.count(contentName))
        {
            Q.push_front({contentName, FIBEntry(false, m, nodeId)});
            iter[contentName] = Q.begin();
        }
        else
        {
            std::string str = extractPrefix(contentName, THRESHOLD);
            if (iter.count(str))
            {
                chmax(iter[str]->second.maximumDepth, m);
            }
            else
            {
                Q.push_front({str, FIBEntry(true, m, {""})});
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
    void save(const FIBPair &fibPair) override
    {
        const std::string contentName = fibPair.getContentName().getValue();

        // LRU 更新
        if (iter.count(contentName) != 0)
        {
            Q.erase(iter[contentName]);
        }
        else if (Q.size() >= MAX_FIB_TABLE_SIZE)
        {
            std::string k = Q.back().first;
            iter.erase(k);
            Q.pop_back();
        }

        int m = std::count(contentName.begin(), contentName.end(), '/');
        SaveFIB(contentName, fibPair.getDestinationId().getValue(), m);

        // fib table すべてを出力する
        // auto begin = fib.begin(), end = fib.end();
        // for (auto iter = begin; iter != end; iter++)
        // {
        //     for (const auto x : iter->second.getNodeId())
        //     {
        //         Serial.printf("%s => %s\n", iter->first.c_str(), x.c_str());
        //     }
        // }
    };

    // 削除は行うタイミングが未定のため詳細は検討すべき
    void remove(const ContentName &contentName) override
    {
        std::string name = contentName.getValue();

        // 実エントリ削除
        if (iter.count(name))
        {
            Q.erase(iter[name]);
            iter.erase(name);
        }

        // 関連する仮想エントリを確認（最大で1つ）
        std::string virtualPrefix = extractPrefix(name, THRESHOLD);
        if (iter.count(virtualPrefix))
        {
            int virtualMD = iter[virtualPrefix]->second.maximumDepth;

            // 仮想のmaximumDepthより深いエントリが他にないか確認
            bool stillNeeded = false;
            for (const auto &p : Q)
            {
                if (p.first != virtualPrefix &&
                    p.first.find(virtualPrefix + "/") == 0) // 仮想プレフィックスの子で
                {
                    int depth = std::count(p.first.begin(), p.first.end(), '/');
                    if (depth > THRESHOLD)
                    {
                        stillNeeded = true;
                        break;
                    }
                }
            }

            if (!stillNeeded)
            {
                Q.erase(iter[virtualPrefix]);
                iter.erase(virtualPrefix);
            }
        }
    };

    bool find(const ContentName &contentName) override
    {
        std::string name = contentName.getValue();
        int n = std::count(name.begin(), name.end(), '/');
        return FIB_LPM_LOOKUP(name, n, THRESHOLD) != nullptr;
    };

    DestinationId get(const ContentName &contentName) override
    {
        std::string name = contentName.getValue();
        int n = std::count(name.begin(), name.end(), '/');
        FIBEntry *result = FIB_LPM_LOOKUP(name, n, THRESHOLD);

        if (result)
        {
            if (iter.count(name))
            {
                Q.splice(Q.begin(), Q, iter[name]);
            }
            else
            {
                Q.push_front({name, *result});
                iter[name] = Q.begin();
            }
            return DestinationId(result->nodeId);
        }

        return DestinationId::Null();
    };
};

#endif // INCLUDED_TWO_STAGE_LOOKUP_FIB_REPOSITORY_hpp_