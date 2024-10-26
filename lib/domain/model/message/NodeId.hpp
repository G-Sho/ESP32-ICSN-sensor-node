#ifndef INCLUDED_NODE_ID_hpp_
#define INCLUDED_NODE_ID_hpp_

#include "Arduino.h"

class NodeId
{
private:
  String value;

public:
  NodeId(String value)
  {
    // 規則を書く

    this->value = value;
  };

  String getValue()
  {
    return value;
  };
};

#endif // INCLUDED_NODE_ID_hpp_