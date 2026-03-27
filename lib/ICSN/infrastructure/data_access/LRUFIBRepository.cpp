#include "LRUFIBRepository.hpp"
#include "../../config/Config.hpp"

// TwoStageアルゴリズム用ヘルパー関数の実装
std::string LRUFIBRepository::extractPrefix(const std::string &name, int prefixDepth) const {
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

bool LRUFIBRepository::lookupEntry(const std::string &name, int prefixDepth, FIBEntry& outEntry) {
    std::string prefix = extractPrefix(name, prefixDepth);
    return cache.get(prefix, outEntry);
}

bool LRUFIBRepository::fibLpmLookup(const std::string &name, int nameDepth, int maxVirtualDepth, FIBEntry& outEntry) {
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

void LRUFIBRepository::saveFibEntry(const std::string &contentName, const std::set<std::string> &nodeIds, int depth) {
    FIBEntry existingEntry;
    if (systemConfig.maxVirtualDepth >= depth) {
        if (cache.get(contentName, existingEntry)) {
            existingEntry.isVirtual = false;
            existingEntry.nodeIds.insert(nodeIds.begin(), nodeIds.end());
            cache.put(contentName, existingEntry);
        } else {
            cache.put(contentName, FIBEntry(false, depth, nodeIds));
        }
    } else {
        std::string virtualPrefix = extractPrefix(contentName, systemConfig.maxVirtualDepth);
        FIBEntry virtualEntry;
        if (cache.get(virtualPrefix, virtualEntry)) {
            chmax(virtualEntry.maximumDepth, depth);
            cache.put(virtualPrefix, virtualEntry);
        } else {
            cache.put(virtualPrefix, FIBEntry(true, depth, {}));
        }
        
        if (cache.get(contentName, existingEntry)) {
            existingEntry.nodeIds.insert(nodeIds.begin(), nodeIds.end());
            cache.put(contentName, existingEntry);
        } else {
            cache.put(contentName, FIBEntry(false, depth, nodeIds));
        }
    }
}

void LRUFIBRepository::save(const FIBPair &fibPair) {
    const std::string &name = fibPair.getContentName().getValue();
    const std::set<std::string> &nodeIds = fibPair.getDestinationId().getValue();
    int depth = std::count(name.begin(), name.end(), '/');
    
    saveFibEntry(name, nodeIds, depth);
}

void LRUFIBRepository::remove(const ContentName &contentName) {
    const std::string &name = contentName.getValue();
    cache.remove(name);
}

bool LRUFIBRepository::find(const ContentName &contentName) {
    const std::string &name = contentName.getValue();
    int nameDepth = std::count(name.begin(), name.end(), '/');
    FIBEntry tempEntry;
    return fibLpmLookup(name, nameDepth, systemConfig.maxVirtualDepth, tempEntry);
}

DestinationId LRUFIBRepository::get(const ContentName &contentName) {
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
