#ifndef INCLUDED_SENDER_ID_hpp_
#define INCLUDED_SENDER_ID_hpp_

#include<string>

class SenderId
{
private:
  std::string value;

public:
  SenderId(std::string value)
  {
    // Write the rules

    this->value = value;
  };

  std::string getValue()
  {
    return value;
  };
};

#endif // INCLUDED_SENDER_ID_hpp_