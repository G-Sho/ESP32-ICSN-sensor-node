#pragma once

#include "message/ContentName.hpp"
#include "message/Content.hpp"

class CSPair
{
private:
  ContentName name;
  Content value;

public:
  CSPair(const ContentName &n, const Content &v) : name(n), value(v) {}
  const ContentName &getContentName() const { return name; };
  const Content &getContent() const { return value; };
};