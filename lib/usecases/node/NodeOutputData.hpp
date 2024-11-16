#ifndef INCLUDED_NODE_OUTPUT_DATA_hpp_
#define INCLUDED_NODE_OUTPUT_DATA_hpp_

#include <string>
#include <vector>

class NodeOutputData
{
private:
  std::string senderId;
  std::vector<std::string> destId;
  std::string signalCode;
  int hopCount;
  std::string contentName;
  std::string content;

public:
  NodeOutputData(std::string senderId, std::vector<std::string> destId, std::string signalCode, int hopCount, std::string contentName, std::string content)
      : senderId(senderId), destId(destId), signalCode(signalCode), hopCount(hopCount), contentName(contentName), content(content) {}

  std::string getSenderId() { return senderId; };

  std::vector<std::string> getDestId() { return destId; };

  std::string getSignalCode() { return signalCode; };

  int getHopCount() { return hopCount; };

  std::string getContentName() { return contentName; };

  std::string getContent() { return content; };
};

#endif // INCLUDED_NODE_OUTPUT_DATA_hpp_