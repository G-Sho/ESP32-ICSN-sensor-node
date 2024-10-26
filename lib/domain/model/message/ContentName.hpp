#ifndef INCLUDED_CONTENT_NAME_hpp_
#define INCLUDED_CONTENT_NAME_hpp_

#include "Arduino.h"

class ContentName
{
private:
  String value;

public:
  ContentName(String value)
  {
    // 規則を書く

    this->value = value;
  };

  String getValue()
  {
    return value;
  };
};

#endif // INCLUDED_CONTENT_NAME_hpp_