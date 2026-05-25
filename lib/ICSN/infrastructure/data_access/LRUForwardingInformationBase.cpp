#include "LRUForwardingInformationBase.hpp"
#include "../../config/Config.hpp"

// TwoStageアルゴリズム用ヘルパー関数の実装
std::string LRUForwardingInformationBase::extractPrefix(const std::string &name, int prefixDepth) const {
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

bool LRUForwardingInformationBase::lookupEntry(const std::string &name, int prefixDepth, FIBEntry& outEntry) {
    std::string prefix = extractPrefix(name, prefixDepth);
    return cache.get(prefix, outEntry);
}

bool LRUForwardingInformationBase::fibLpmLookup(const std::string &name, int nameDepth, int maxVirtualDepth, FIBEntry& outEntry) {
    FIBEntry fibEntry;
    FIBEntry fibEntry1s;
    int startPfx;
    bool stage2Flag = false;
    
    // FIB_LPMステージ1
    for (int pfx = std::min(nameDepth, maxVirtualDepth); pfx >= 1; --pfx) {
        if (lookupEntry(name, pfx, fibEntry)) {
            if (fibEntry.maximumDepth > maxVirtualDepth && nameDepth > maxVirtualDepth) {
                startPfx = std::min(nameDepth, fibEntry.maximumDepth);
                fibEntry1s = fibEntry;
                stage2Flag = true;
                break;
            } else if (!fibEntry.isVirtual) {
                outEntry = fibEntry;
                return true;
            }
        }
    }
    
    // FIB_LPMステージ2
    if (stage2Flag) {
        for (int pfx = startPfx; pfx >= 1; --pfx) {
            if (pfx == maxVirtualDepth) {
                if (!fibEntry1s.isVirtual) {
                    outEntry = fibEntry1s;
                    return true;
                }
            }
            if (lookupEntry(name, pfx, fibEntry) && !fibEntry.isVirtual) {
                outEntry = fibEntry;
                return true;
            }
        }
    }
    return false;
}

void LRUForwardingInformationBase::save(const FIBPair &fibPair) {
    const std::string &name = fibPair.getContentName().getValue();
    const std::set<std::string> &nodeIds = fibPair.getDestinationId().getValue();
    int depth = std::count(name.begin(), name.end(), '/');

    FIBEntry existing;
    if (cache.get(name, existing)) {
        existing.isVirtual = false;
        existing.nodeIds.insert(nodeIds.begin(), nodeIds.end());
        cache.put(name, existing);
    } else {
        cache.put(name, FIBEntry(false, depth, nodeIds));
    }
}

void LRUForwardingInformationBase::saveVirtualEntry(const ContentName &prefix, int maximumDepth) {
    const std::string &name = prefix.getValue();
    FIBEntry existing;
    if (cache.get(name, existing)) {
        chmax(existing.maximumDepth, maximumDepth);
        cache.put(name, existing);
    } else {
        cache.put(name, FIBEntry(true, maximumDepth, {}));
    }
}

void LRUForwardingInformationBase::remove(const ContentName &contentName) {
    const std::string &name = contentName.getValue();
    cache.remove(name);
}

bool LRUForwardingInformationBase::find(const ContentName &contentName) {
    const std::string &name = contentName.getValue();
    int nameDepth = std::count(name.begin(), name.end(), '/');
    FIBEntry tempEntry;
    return fibLpmLookup(name, nameDepth, systemConfig.maxVirtualDepth, tempEntry);
}

DestinationId LRUForwardingInformationBase::get(const ContentName &contentName) {
    const std::string &name = contentName.getValue();
    int nameDepth = std::count(name.begin(), name.end(), '/');
    FIBEntry result;
    
    if (fibLpmLookup(name, nameDepth, systemConfig.maxVirtualDepth, result)) {
        // LRUキャッシュを更新（アクセス順序を更新）
        cache.put(name, result);
        return DestinationId(result.nodeIds);
    }
    
    return DestinationId::Null();
}
