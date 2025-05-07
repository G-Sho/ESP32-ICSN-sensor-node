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

  std::string getValue()
  {
    return value;
  };
};

#endif // INCLUDED_CONTENT_hpp_