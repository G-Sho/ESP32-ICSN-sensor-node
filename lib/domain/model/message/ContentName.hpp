#ifndef INCLUDED_CONTENT_NAME_hpp_
#define INCLUDED_CONTENT_NAME_hpp_

#include <string>

class ContentName
{
private:
  std::string value;

public:
  ContentName(std::string value)
  {
    // Write the rules

    this->value = value;
  };

  static ContentName Null()
  {
    return ContentName({});
  }

  bool isNull() const
  {
    return value.empty();
  }

  std::string getValue() const
  {
    return value;
  };
};

#endif // INCLUDED_CONTENT_NAME_hpp_