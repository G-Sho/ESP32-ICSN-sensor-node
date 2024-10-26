#ifndef INCLUDED_NODE_ADD_OUTPUT_DATA_hpp_
#define INCLUDED_NODE_ADD_OUTPUT_DATA_hpp_

#include "Arduino.h"

class NodeAddOutputData
{
private:
  String senderId;
  String destId;
  String signalCode;
  int hopCount;
  String contentName;
  String content;

public:
  NodeAddOutputData(String senderId, String destId, String signalCode, int hopCount, String contentName, String content)
      : senderId(senderId), destId(destId), signalCode(signalCode), hopCount(hopCount), contentName(contentName), content(content) {}

  String getSenderId() { return senderId; };

  String getDestId() { return destId; };

  String getSignalCode() { return signalCode; };

  int getHopCount() { return hopCount; };

  String getContentName() { return contentName; };

  String getContent() { return content; };
};

#endif // INCLUDED_NODE_ADD_OUTPUT_DATA_hpp_