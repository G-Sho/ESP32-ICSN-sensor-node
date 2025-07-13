#ifndef INCLUDED_NODE_OUTPUT_DATA_hpp_
#define INCLUDED_NODE_OUTPUT_DATA_hpp_

#include <string>
#include <set>

class NodeOutputData
{
private:
  std::string senderId;
  std::set<std::string> destId;
  std::string signalCode;
  int hopCount;
  std::string contentName;
  std::string content;
  uint32_t time;

public:
  NodeOutputData(std::string senderId, std::set<std::string> destId, std::string signalCode, int hopCount, std::string contentName, std::string content, uint32_t time)
      : senderId(senderId), destId(destId), signalCode(signalCode), hopCount(hopCount), contentName(contentName), content(content), time(time) {}

  std::string getSenderId() { return senderId; };

  std::set<std::string> getDestId() { return destId; };

  std::string getSignalCode() { return signalCode; };

  int getHopCount() { return hopCount; };

  std::string getContentName() { return contentName; };

  std::string getContent() { return content; };

  uint32_t getTime() { return time; };
};

#endif // INCLUDED_NODE_OUTPUT_DATA_hpp_