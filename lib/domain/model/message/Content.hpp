#ifndef INCLUDED_CONTENT_hpp_
#define INCLUDED_CONTENT_hpp_

class Content
{
private:
  std::string value;

public:
  Content(std::string value)
  {
    // 規則を書く

    this->value = value;
  };

  std::string getValue()
  {
    return value;
  };
};

#endif // INCLUDED_CONTENT_hpp_