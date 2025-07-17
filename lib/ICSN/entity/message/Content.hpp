#pragma once

#include <string>

class Content
{
private:
  std::pair<std::string, uint32_t> value;

public:
  Content(const std::pair<std::string, uint32_t> &v) : value(v) {}
  static Content Null() { return Content({}); }
  const std::pair<std::string, uint32_t> &getValue() const { return value; };
};