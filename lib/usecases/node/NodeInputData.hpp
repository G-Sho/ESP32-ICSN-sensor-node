#ifndef INCLUDED_NODE_INPUT_DATA_hpp_
#define INCLUDED_NODE_INPUT_DATA_hpp_

#include <string>

class NodeInputData
{
private:
  std::string senderId;
  std::string destId;
  std::string signalCode;
  int hopCount;
  std::string contentName;
  std::string content;

public:
  NodeInputData(std::string senderId, std::string destId, std::string signalCode, int hopCount, std::string contentName, std::string content)
      : senderId(senderId), destId(destId), signalCode(signalCode), hopCount(hopCount), contentName(contentName), content(content) {}

  std::string getSenderId() { return senderId; };

  std::string getDestId() { return destId; };

  std::string getSignalCode() { return signalCode; };

  int getHopCount() { return hopCount; };

  std::string getContentName() { return contentName; };

  std::string getContent() { return content; };
};

#endif // INCLUDED_NODE_INPUT_DATA_hpp_