#ifndef INCLUDED_NODE_OUTPUT_DATA_hpp_
#define INCLUDED_NODE_OUTPUT_DATA_hpp_

class NodeOutputData
{
private:
  String senderId;
  String destId;
  String signalCode;
  int hopCount;
  String contentName;
  String content;

public:
  NodeOutputData(String senderId, String destId, String signalCode, int hopCount, String contentName, String content)
      : senderId(senderId), destId(destId), signalCode(signalCode), hopCount(hopCount), contentName(contentName), content(content) {}

  String getSenderId() { return senderId; };

  String getDestId() { return destId; };

  String getSignalCode() { return signalCode; };

  int getHopCount() { return hopCount; };

  String getContentName() { return contentName; };

  String getContent() { return content; };
};

#endif // INCLUDED_NODE_OUTPUT_DATA_hpp_