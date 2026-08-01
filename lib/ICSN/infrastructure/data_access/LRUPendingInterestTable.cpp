#include "LRUPendingInterestTable.hpp"
#include "../../BuildProfile.hpp"
#include "../../config/Config.hpp"

void LRUPendingInterestTable::save(const PITPair& pitPair) {
  const std::string& name = pitPair.getContentName().getValue();
  RequesterSet mergedDestinations;
  size_t dropped = 0;

  RequesterSet existingDestinations;
  if (cache.get(name, existingDestinations)) {
    dropped += mergedDestinations.insertFromOther(existingDestinations);
  }

  dropped += mergedDestinations.insertFromStdSet(pitPair.getDestinationId().getValue());

  cache.put(name, mergedDestinations);
  LOG_DEBUGF("[DEBUG][PIT] entry_saved name=%s requesters=%u\n", name.c_str(),
             static_cast<unsigned int>(mergedDestinations.size()));

  if (dropped > 0) {
    LOG_WARNF(
        "[WARN][PIT] entry_save_partial name=%s reason=requester_capacity_exceeded dropped=%u\n",
        name.c_str(), static_cast<unsigned int>(dropped));
  }
}

void LRUPendingInterestTable::remove(const ContentName& contentName) {
  const std::string& name = contentName.getValue();
  if (cache.contains(name)) {
    cache.remove(name);
    LOG_DEBUGF("[DEBUG][PIT] entry_removed name=%s\n", name.c_str());
  }
}

bool LRUPendingInterestTable::find(const ContentName& contentName) {
  const std::string& name = contentName.getValue();
  const bool found = cache.contains(name);
  LOG_DEBUGF("[DEBUG][PIT] %s name=%s\n", found ? "lookup_hit" : "lookup_miss", name.c_str());
  return found;
}

DestinationId LRUPendingInterestTable::get(const ContentName& contentName) {
  const std::string& name = contentName.getValue();
  RequesterSet destinations;

  if (cache.get(name, destinations)) {
    return DestinationId(destinations.toStdSet());
  }

  return DestinationId::Null();
}
