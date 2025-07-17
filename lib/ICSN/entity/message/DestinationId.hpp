#pragma once

#include <string>
#include <set>

class DestinationId
{
private:
  std::set<std::string> ids;

public:
  DestinationId(const std::set<std::string> &s) : ids(s) {}
  const std::set<std::string> &getValue() const { return ids; }
  static DestinationId Null() { return DestinationId({}); }
  bool isNull() const { return ids.empty(); }
  bool operator==(const DestinationId &other) const { return ids == other.ids; }
};