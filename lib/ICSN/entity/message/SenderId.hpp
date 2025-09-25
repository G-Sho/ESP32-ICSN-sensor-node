#pragma once

#include <string>

class SenderId
{
private:
  std::string id;

public:
  SenderId(const std::string &i) : id(i) {}
  const std::string &getValue() const { return id; };
};