#include "TwoStageLookupFIBRepository.hpp"
#include "config/Config.hpp"

void TwoStageLookupFIBRepository::save(const FIBPair &fibPair)
{
    const std::string contentName = fibPair.getContentName().getValue();

    // LRU 更新
    if (iter.count(contentName) != 0)
    {
        Q.erase(iter[contentName]);
    }
    else if (Q.size() >= systemConfig.maxFibTableSize)
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
void TwoStageLookupFIBRepository::remove(const ContentName &contentName)
{
    std::string name = contentName.getValue();

    // 実エントリ削除
    if (iter.count(name))
    {
        Q.erase(iter[name]);
        iter.erase(name);
    }

    // 関連する仮想エントリを確認（最大で1つ）
    std::string virtualPrefix = extractPrefix(name, systemConfig.maxVirtualDepth);
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
                if (depth > systemConfig.maxVirtualDepth)
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

bool TwoStageLookupFIBRepository::find(const ContentName &contentName)
{
    std::string name = contentName.getValue();
    int n = std::count(name.begin(), name.end(), '/');
    return FIB_LPM_LOOKUP(name, n, systemConfig.maxVirtualDepth) != nullptr;
};

DestinationId TwoStageLookupFIBRepository::get(const ContentName &contentName)
{
    std::string name = contentName.getValue();
    int n = std::count(name.begin(), name.end(), '/');
    FIBEntry *result = FIB_LPM_LOOKUP(name, n, systemConfig.maxVirtualDepth);

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