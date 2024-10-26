#ifndef INCLUDED_NUMBER_OF_HOPS_hpp_
#define INCLUDED_NUMBER_OF_HOPS_hpp_

#include "Arduino.h"

class HopCount
{
private:
  int value;

public:
  HopCount(int value)
  {
    // 規則を書く

    this->value = value;
  };

  int getValue()
  {
    return value;
  };

  void increment()
  {
    value++;
  };
};

#endif // INCLUDED_NUMBER_OF_HOPS_hpp_