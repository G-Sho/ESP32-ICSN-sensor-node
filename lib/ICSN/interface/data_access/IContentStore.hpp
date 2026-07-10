#pragma once

#include "../entity/message/Content.hpp"
#include "../entity/message/ContentName.hpp"
#include "../entity/routing_table/CSPair.hpp"

class IContentStore {
public:
  virtual void save(const CSPair& csPair) = 0;
  virtual void remove(const ContentName& contentName) = 0;
  virtual bool find(const ContentName& contentName) = 0;
  virtual Content get(const ContentName& contentName) = 0;
  virtual void clear() = 0;

  virtual ~IContentStore() = default;
};
