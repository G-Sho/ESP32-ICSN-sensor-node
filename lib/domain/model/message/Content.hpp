#ifndef INCLUDED_CONTENT_hpp_
#define INCLUDED_CONTENT_hpp_

#include<string>

class Content
{
private:
  std::pair<std::string, uint32_t> value;

public:
  Content(std::pair<std::string, uint32_t> value)
  {
    // Write the rules

    this->value = value;
  };

  static Content Null()
  {
    return Content({});
  }

  std::pair<std::string, uint32_t> getValue() const
  {
    return value;
  };
};

#endif // INCLUDED_CONTENT_hpp_