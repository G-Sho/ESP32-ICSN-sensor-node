#ifndef INCLUDED_CONTENT_hpp_
#define INCLUDED_CONTENT_hpp_

#include "Arduino.h"

class Content
{
private:
  String value;

public:
  Content(String value)
  {
    // 規則を書く

    this->value = value;
  };

  String getValue()
  {
    return value;
  };
};

#endif // INCLUDED_CONTENT_hpp_