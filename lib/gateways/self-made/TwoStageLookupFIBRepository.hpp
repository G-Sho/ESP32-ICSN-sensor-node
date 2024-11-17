#ifndef INCLUDED_TWO_STAGE_LOOKUP_FIB_REPOSITORY_hpp_
#define INCLUDED_TWO_STAGE_LOOKUP_FIB_REPOSITORY_hpp_

#include "interface/FIBRepository.hpp"
#include <map>       //std::map
#include <algorithm> // std::min

#define THRESHOLD 5 // システム定義の閾値

class TwoStageLookupFIBRepository : public FIBRepository
{
private:
    // FIBエントリの構造体
    struct FIBEntry
    {
        bool isVir;
        int maximumDepth;
        std::vector<std::string> nodeId;
        FIBEntry(bool flg, int num, std::vector<std::string> Id)
        {
            isVir = flg;
            maximumDepth = num;
            nodeId = Id;
        }
    };

    std::map<std::string, FIBEntry> fib{
        {"/humid", FIBEntry(false, 1, {"1553658797"})},
        {"/temp", FIBEntry(false, 1, {"1553658821"})}};

    // FIBを検索する関数
    FIBEntry *LookupFIB(const std::string &name, int pfx)
    {
        // 実際のFIB検索処理をここに実装
        int num = 0;
        std::string str = name;
        for (int i = 0; i <= name.length(); i++)
        {
            if (name[i] == '/')
            {
                num++;
                if (num > pfx)
                {
                    str = name.substr(0, i);
                    break;
                }
            }
        }
        auto iter = fib.find(str);
        if (iter != end(fib))
        {
            return &fib.at(str);
        }
        else
        {
            return nullptr; // 仮の戻り値としてnullを返すF
        }
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
        };
        return nullptr; // FIB_NOT_FOUND
    }

public:
    void save(FIBPair fibPair) override {
        // mijissou
    };

    void remove(ContentName contentName) override {
        // mijissou
    };

    bool find(ContentName contentName) override
    {
        int n = 0; // number of name components in an Interest packet
        for (int i = 0; i < contentName.getValue().length(); i++)
            if (contentName.getValue()[i] == '/')
                n++;
        FIBEntry *result = FIB_LPM_LOOKUP(contentName.getValue(), n, THRESHOLD);
        if (result)
            return true;
        else
            return false;
    };

    DestinationId get(ContentName contentName) override
    {
        int n = 0; // number of name components in an Interest packet
        for (int i = 0; i < contentName.getValue().length(); i++)
            if (contentName.getValue()[i] == '/')
                n++;
        FIBEntry *result = FIB_LPM_LOOKUP(contentName.getValue(), n, THRESHOLD);
        if (result)
            return DestinationId(result->nodeId);
        else
            return DestinationId({"NULL"});
    };
};

#endif // INCLUDED_TWO_STAGE_LOOKUP_FIB_REPOSITORY_hpp_