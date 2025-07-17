#pragma once

#include <string>

class ContentName
{
private:
  std::string name;

public:
  ContentName(const std::string &v) : name(v) {}

  static ContentName Null() { return ContentName({}); }

  bool isNull() const { return name.empty(); }

  std::string getValue() const { return name; }
};