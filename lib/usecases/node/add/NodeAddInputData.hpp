#ifndef INCLUDED_NODE_ADD_INPUT_DATA_hpp_
#define INCLUDED_NODE_ADD_INPUT_DATA_hpp_

class NodeAddInputData
{
private:
  String senderId;
  String destId;
  String signalCode;
  int hopCount;
  String contentName;
  String content;

public:
  NodeAddInputData(String senderId, String destId, String signalCode, int hopCount, String contentName, String content)
      : senderId(senderId), destId(destId), signalCode(signalCode), hopCount(hopCount), contentName(contentName), content(content) {}

  String getSenderId() { return senderId; };

  String getDestId() { return destId; };

  String getSignalCode() { return signalCode; };

  int getHopCount() { return hopCount; };

  String getContentName() { return contentName; };

  String getContent() { return content; };
};

#endif // INCLUDED_NODE_ADD_INPUT_DATA_hpp_