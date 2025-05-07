#ifndef INCLUDED_DESTINATION_ID_hpp_
#define INCLUDED_DESTINATION_ID_hpp_

#include<string>
#include <set>

class DestinationId
{
private:
  std::set<std::string> value;

public:
  DestinationId(std::set<std::string> value)
  {
    // Write the rules

    this->value = value;
  };

  std::set<std::string> getValue()
  {
    return value;
  };
};

#endif // INCLUDED_DESTINATION_ID_hpp_