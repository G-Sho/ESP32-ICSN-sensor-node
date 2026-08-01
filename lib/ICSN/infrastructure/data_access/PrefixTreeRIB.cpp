#include "PrefixTreeRIB.hpp"
#include "../../BuildProfile.hpp"
#include "../../entity/message/ContentName.hpp"
#include "../../entity/message/DestinationId.hpp"
#include "../../entity/routing_table/FIBPair.hpp"
#include <algorithm>

PrefixTreeRIB::PrefixTreeRIB(IForwardingInformationBase& fib) : fibRepository(fib) {}

std::string PrefixTreeRIB::extractPrefix(const std::string& name, int prefixDepth) {
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

void PrefixTreeRIB::addRoute(const std::string& contentName, const std::string& nextHopId) {
  // RIB ノード上限チェック
  if (tree.find(contentName) == tree.end() && tree.size() >= BuildCapacity::RIB_ENTRIES) {
    LOG_WARNF("[WARN][RIB] route_add_failed name=%s reason=capacity_full capacity=%u\n",
              contentName.c_str(), static_cast<unsigned int>(BuildCapacity::RIB_ENTRIES));
    return;
  }

  // RIB を更新
  RIBEntry& node = tree[contentName];
  node.isReal = true;
  if (!node.nextHopIds.insert(nextHopId)) {
    LOG_WARNF(
        "[WARN][RIB] route_add_partial name=%s reason=next_hop_capacity_exceeded next_hop=%s\n",
        contentName.c_str(), nextHopId.c_str());
  }

  LOG_INFOF("[INFO][RIB] route_added name=%s next_hop=%s\n", contentName.c_str(),
            nextHopId.c_str());

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

void PrefixTreeRIB::printUsageStats() const {
  CLI_PRINTF("[RIB] entries=%u/%u, next_hops_per_node=%u\n", static_cast<unsigned int>(tree.size()),
             static_cast<unsigned int>(BuildCapacity::RIB_ENTRIES),
             static_cast<unsigned int>(BuildCapacity::RIB_NEXT_HOPS_PER_NODE));
}

void PrefixTreeRIB::removeRoute(const std::string& contentName) {
  tree.erase(contentName);
  fibRepository.remove(ContentName(contentName));
  LOG_DEBUGF("[DEBUG][RIB] route_removed name=%s\n", contentName.c_str());

  // virtual prefix が他のエントリで不要になった場合は FIB からも削除
  int depth = std::count(contentName.begin(), contentName.end(), '/');
  if (depth > systemConfig.maxVirtualDepth) {
    std::string virtualPrefix = extractPrefix(contentName, systemConfig.maxVirtualDepth);
    bool prefixStillNeeded = false;
    for (const auto& kv : tree) {
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
