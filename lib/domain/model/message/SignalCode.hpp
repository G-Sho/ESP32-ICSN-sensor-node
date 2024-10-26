#ifndef INCLUDED_SIGNAL_CODE_hpp_
#define INCLUDED_SIGNAL_CODE_hpp_

#include "Arduino.h"

class SignalCode
{
private:
  String value;

public:
  SignalCode(String value)
  {
    // 規則を書く

    this->value = value;
  };

  String getValue()
  {
    return value;
  };
};

#endif // INCLUDED_SIGNAL_CODE_h_