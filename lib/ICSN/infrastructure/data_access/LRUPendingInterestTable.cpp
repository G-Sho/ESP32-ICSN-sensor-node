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

  if (dropped > 0) {
    LOG_WARNF("[PIT] requester capacity exceeded for key=%s, dropped=%u\n", name.c_str(),
              static_cast<unsigned int>(dropped));
  }
}

void LRUPendingInterestTable::remove(const ContentName& contentName) {
  const std::string& name = contentName.getValue();
  cache.remove(name);

  LOG_DEBUGF("Removed PIT entry: Key=%s\n", name.c_str());
}

bool LRUPendingInterestTable::find(const ContentName& contentName) {
  return cache.contains(contentName.getValue());
}

DestinationId LRUPendingInterestTable::get(const ContentName& contentName) {
  const std::string& name = contentName.getValue();
  RequesterSet destinations;

  if (cache.get(name, destinations)) {
    return DestinationId(destinations.toStdSet());
  }

  return DestinationId::Null();
}
