#ifndef INCLUDED_SIGNAL_CODE_hpp_
#define INCLUDED_SIGNAL_CODE_hpp_

#include "Arduino.h"

class SignalCode
{
private:
  std::string value;

public:
  SignalCode(std::string value)
  {
    // 規則を書く

    this->value = value;
  };

  std::string getValue()
  {
    return value;
  };
};

#endif // INCLUDED_SIGNAL_CODE_h_