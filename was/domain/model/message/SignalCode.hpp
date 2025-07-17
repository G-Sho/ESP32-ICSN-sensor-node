#ifndef INCLUDED_SIGNAL_CODE_hpp_
#define INCLUDED_SIGNAL_CODE_hpp_

#include<string>

class SignalCode
{
private:
  std::string value;

public:
  SignalCode(std::string value)
  {
    // Write the rules

    this->value = value;
  };

  std::string getValue()
  {
    return value;
  };
};

#endif // INCLUDED_SIGNAL_CODE_h_