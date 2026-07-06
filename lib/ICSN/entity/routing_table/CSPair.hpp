#pragma once

#include "message/Content.hpp"
#include "message/ContentName.hpp"

class CSPair {
private:
  ContentName name;
  Content value;

public:
  CSPair(const ContentName& n, const Content& v) : name(n), value(v) {}
  const ContentName& getContentName() const {
    return name;
  };
  const Content& getContent() const {
    return value;
  };
};
