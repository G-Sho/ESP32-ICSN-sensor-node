#include "PrefixTreeRIB.hpp"
#include "../../entity/routing_table/FIBPair.hpp"
#include "../../entity/message/ContentName.hpp"
#include "../../entity/message/DestinationId.hpp"
#include <algorithm>

PrefixTreeRIB::PrefixTreeRIB(IForwardingInformationBase &fib)
    : fibRepository(fib)
{
}

std::string PrefixTreeRIB::extractPrefix(const std::string &name, int prefixDepth)
{
    int cnt = 0;
    for (size_t i = 0; i < name.length(); i++) {
        if (name[i] == '/') {
            cnt++;
            if (cnt > prefixDepth) {
                return name.substr(0, i);
            }
        }
    }
    return name;
}

void PrefixTreeRIB::addRoute(const std::string &contentName, const std::string &nextHopId)
{
    // RIB ノード上限チェック
    if (tree.find(contentName) == tree.end() && tree.size() >= MAX_RIB_NODE_SIZE) {
        return;
    }

    // RIB を更新
    RIBNode &node = tree[contentName];
    node.isReal = true;
    node.nextHopIds.insert(nextHopId);

    int depth = std::count(contentName.begin(), contentName.end(), '/');

    if (depth <= systemConfig.maxVirtualDepth) {
        // depth ≤ M: FIB に real entry を保存
        FIBPair fibPair(ContentName(contentName), DestinationId({nextHopId}));
        fibRepository.save(fibPair);
    } else {
        // depth > M: virtual prefix entry を保存し、フルパス real entry も保存
        std::string virtualPrefix = extractPrefix(contentName, systemConfig.maxVirtualDepth);
        fibRepository.saveVirtualEntry(ContentName(virtualPrefix), depth);

        FIBPair fibPair(ContentName(contentName), DestinationId({nextHopId}));
        fibRepository.save(fibPair);
    }
}

void PrefixTreeRIB::removeRoute(const std::string &contentName)
{
    tree.erase(contentName);
    fibRepository.remove(ContentName(contentName));

    // virtual prefix が他のエントリで不要になった場合は FIB からも削除
    int depth = std::count(contentName.begin(), contentName.end(), '/');
    if (depth > systemConfig.maxVirtualDepth) {
        std::string virtualPrefix = extractPrefix(contentName, systemConfig.maxVirtualDepth);
        bool prefixStillNeeded = false;
        for (const auto &kv : tree) {
            int d = std::count(kv.first.begin(), kv.first.end(), '/');
            if (d > systemConfig.maxVirtualDepth &&
                extractPrefix(kv.first, systemConfig.maxVirtualDepth) == virtualPrefix) {
                prefixStillNeeded = true;
                break;
            }
        }
        if (!prefixStillNeeded) {
            fibRepository.remove(ContentName(virtualPrefix));
        }
    }
}
