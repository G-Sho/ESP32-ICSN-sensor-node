#pragma once

#include "model/message/ContentName.hpp"
#include "model/message/Content.hpp"
#include "model/message/Time.hpp"

class CSPair
{
private:
  ContentName name;
  Content value;

public:
  CSPair(const ContentName &n, const Content &v) : name(n), value(v) {}
  
  const ContentName getContentName() const { return name; };

  const Content getContent() const { return value; };
};