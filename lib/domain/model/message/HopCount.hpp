#ifndef INCLUDED_NUMBER_OF_HOPS_hpp_
#define INCLUDED_NUMBER_OF_HOPS_hpp_

#include<string>

class HopCount
{
private:
  int value;

public:
  HopCount(int value)
  {
    // Write the rules

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