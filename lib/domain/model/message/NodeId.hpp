#ifndef INCLUDED_NODE_ID_hpp_
#define INCLUDED_NODE_ID_hpp_

#include<string>

class NodeId
{
private:
  std::string value;

public:
  NodeId(std::string value)
  {
    // Write the rules

    this->value = value;
  };

  std::string getValue()
  {
    return value;
  };
};

#endif // INCLUDED_NODE_ID_hpp_