#ifndef INCLUDED_DESTINATION_ID_hpp_
#define INCLUDED_DESTINATION_ID_hpp_

#include<string>
#include <vector>

class DestinationId
{
private:
  std::vector<std::string> value;

public:
  DestinationId(std::vector<std::string> value)
  {
    // 規則を書く

    this->value = value;
  };

  std::vector<std::string> getValue()
  {
    return value;
  };
};

#endif // INCLUDED_DESTINATION_ID_hpp_