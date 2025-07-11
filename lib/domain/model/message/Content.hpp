#ifndef INCLUDED_CONTENT_hpp_
#define INCLUDED_CONTENT_hpp_

#include<string>

class Content
{
private:
  std::string value;

public:
  Content(std::string value)
  {
    // Write the rules

    this->value = value;
  };

  static Content Null()
  {
    return Content({});
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

#endif // INCLUDED_CONTENT_hpp_