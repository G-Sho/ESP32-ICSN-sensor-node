#ifndef INCLUDED_CONTENT_NAME_hpp_
#define INCLUDED_CONTENT_NAME_hpp_

class ContentName
{
private:
  std::string value;

public:
  ContentName(std::string value)
  {
    // 規則を書く

    this->value = value;
  };

  std::string getValue()
  {
    return value;
  };
};

#endif // INCLUDED_CONTENT_NAME_hpp_