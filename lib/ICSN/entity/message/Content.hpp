#pragma once

#include <string>

class Content
{
private:
  std::string value;

public:
  Content(const std::string &v) : value(v) {}
  static Content Null() { return Content(""); }
  const std::string &getValue() const { return value; };
};