#include "TwoStageLookupFIBRepository.hpp"
#include "config/Config.hpp"

void TwoStageLookupFIBRepository::save(const FIBPair &fibPair)
{
    const std::string contentName = fibPair.getContentName().getValue();

    // LRU 更新
    if (iter.count(contentName) != 0)
    {
        Q.erase(iter[contentName]);
        iter.erase(contentName);  // イテレータマップからも削除
    }
    else if (Q.size() >= systemConfig.maxFibTableSize)
    {
        std::string k = Q.back().first;
        iter.erase(k);
        Q.pop_back();
    }

    int m = std::count(contentName.begin(), contentName.end(), '/');
    SaveFIB(contentName, fibPair.getDestinationId().getValue(), m);

    // Print cache contents
    // Serial.printf("Saved: Key=%s, Value: {", contentName.c_str());
    // for (const auto &id : fibPair.getDestinationId().getValue())
    // {
    //     Serial.printf("%s ", id.c_str());
    // }
    // Serial.printf("}\n");
    // printCache();
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

    // Print cache contents after removal
    // Serial.printf("Removed: Key=%s\n", name.c_str());
    // printCache();
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

    // Print cache contents after retrieval
    // Serial.printf("Retrieved: Key=%s, Value: {", name.c_str());
    // if (iter.count(name))
    // {
    //     for (const auto &id : iter[name]->second.nodeId)
    //     {
    //         Serial.printf("%s ", id.c_str());
    //     }
    //     Serial.printf("}\n");
    //     return DestinationId(iter[name]->second.nodeId);
    // }
    // Serial.printf("}\n");
    // printCache();

    return DestinationId::Null();
};