#ifndef INCLUDED_MESSAGE_CONTROLLER_hpp_
#define INCLUDED_MESSAGE_CONTROLLER_hpp_

#include <Arduino_JSON.h>
#include "node\NodeInteractor.hpp"
#include "node\NodeInputData.hpp"
#include "node\NodeOutputData.hpp"

// SIGNAL
#define SIGNAL_INTEREST '1' // Interest
#define SIGNAL_DATA '2'     // Data
#define SIGANAL_INVALID '3' // Invalid message

class NodeController
{
private:
  NodeInteractor nodeInteractor;

public:
  String receiveMessage(uint32_t to, String msg)
  {
    JSONVar myObject = JSON.parse(msg.c_str());
    String senderId = JSON.stringify(myObject["senderId"]).substring(1, JSON.stringify(myObject["senderId"]).length() - 1); // chipId of sender
    String destId = JSON.stringify(myObject["destId"]).substring(1, JSON.stringify(myObject["destId"]).length() - 1);       // chipId of destination
    if (destId == "-1")
      destId = String((int)to); // Broadcast notoki -1 dakara naosu
    String signalCode = JSON.stringify(myObject["signalCode"]).substring(1, JSON.stringify(myObject["signalCode"]).length() - 1);

    switch (signalCode[0])
    {
    case SIGNAL_INTEREST:
    {
      int hopCount = myObject["hopCount"];
      String contentName = JSON.stringify(myObject["contentName"]).substring(1, JSON.stringify(myObject["contentName"]).length() - 1);
      String content = JSON.stringify(myObject["content"]).substring(1, JSON.stringify(myObject["content"]).length() - 1);
      NodeInputData inputData = NodeInputData(senderId, destId, signalCode, hopCount, contentName, content);
      NodeOutputData outputData = (nodeInteractor.handleInterestReceive(inputData));
      JSONVar jsonData;
      jsonData["destId"] = outputData.getDestId();
      jsonData["senderId"] = outputData.getSenderId();
      jsonData["signalCode"] = outputData.getSignalCode();
      jsonData["hopCount"] = outputData.getHopCount();
      jsonData["contentName"] = outputData.getContentName();
      jsonData["content"] = outputData.getContent();
      String data = JSON.stringify(jsonData);
      return data;
      break;
    }
    case SIGNAL_DATA:
    {
      int hopCount = myObject["hopCount"];
      String contentName = JSON.stringify(myObject["contentName"]).substring(1, JSON.stringify(myObject["contentName"]).length() - 1);
      String content = JSON.stringify(myObject["content"]).substring(1, JSON.stringify(myObject["content"]).length() - 1);
      NodeInputData inputData = NodeInputData(senderId, destId, signalCode, hopCount, contentName, content);
      NodeOutputData outputData = (nodeInteractor.handleDataReceive(inputData));
      JSONVar jsonData;
      jsonData["destId"] = outputData.getDestId();
      jsonData["senderId"] = outputData.getSenderId();
      jsonData["signalCode"] = outputData.getSignalCode();
      jsonData["hopCount"] = outputData.getHopCount();
      jsonData["contentName"] = outputData.getContentName();
      jsonData["content"] = outputData.getContent();
      String data = JSON.stringify(jsonData);
      return data;
      break;
    }
    default:
      break;
    }
  };

  void reciveSensorData(String msg)
  {
    JSONVar myObject = JSON.parse(msg.c_str());
    String senderId = JSON.stringify(myObject["senderId"]).substring(1, JSON.stringify(myObject["senderId"]).length() - 1); // chipId of sender
    String destId = JSON.stringify(myObject["destId"]).substring(1, JSON.stringify(myObject["destId"]).length() - 1);       // chipId of destination
    String signalCode = JSON.stringify(myObject["signalCode"]).substring(1, JSON.stringify(myObject["signalCode"]).length() - 1);
    String contentName = JSON.stringify(myObject["contentName"]).substring(1, JSON.stringify(myObject["contentName"]).length() - 1);
    String content = JSON.stringify(myObject["content"]).substring(1, JSON.stringify(myObject["content"]).length() - 1);

    NodeInputData inputData = NodeInputData(senderId, destId, signalCode, 0, contentName, content);
    nodeInteractor.handleSensorDataReceive(inputData);
  }
};

#endif // INCLUDED_MESSAGE_CONTROLLER_hpp_