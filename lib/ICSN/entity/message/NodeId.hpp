#pragma once

#include <string>

class NodeId
{
private:
  std::string value;

public:
  NodeId(const std::string &value) : value(value) {}
  const std::string &getValue() const { return value; };
};