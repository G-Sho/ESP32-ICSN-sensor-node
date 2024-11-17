#ifndef INCLUDED_Arduino_CONTROLLER_hpp_
#define INCLUDED_Arduino_CONTROLLER_hpp_

#include <ArduinoJSON.h>
#include <string>
#include "node\NodeInteractor.hpp"
#include "node\NodeInputData.hpp"
#include "node\NodeOutputData.hpp"

// SIGNAL
#define SIGNAL_INTEREST "1"   // Interest
#define SIGNAL_DATA "2"       // Data
#define SIGANAL_INVALID "3"   // Invalid message

class ArduinoController
{
private:
  NodeInteractor nodeInteractor;

public:
  String receiveMessage(uint32_t to, String msg)
  {
    StaticJsonDocument<200> inputDoc;

    DeserializationError error = deserializeJson(inputDoc, msg);

    if (error)
    {
      Serial.print("Deserialization failure: ");
      Serial.println(error.c_str());
      exit(0);
    }

    std::string senderId = inputDoc["senderId"] | "unknown";
    std::string destId = std::to_string(to);
    std::string signalCode = inputDoc["signalCode"] | "unknown";

    if (signalCode == SIGNAL_INTEREST)
    {
      int hopCount = inputDoc["hopCount"];
      std::string contentName = inputDoc["contentName"];
      std::string content = inputDoc["content"];
      NodeInputData inputData = NodeInputData(senderId, destId, signalCode, hopCount, contentName, content);

      NodeOutputData outputData = (nodeInteractor.handleInterestReceive(inputData));

      StaticJsonDocument<200> outputDoc;
      outputDoc["senderId"] = outputData.getSenderId();
      JsonArray destId = outputDoc.createNestedArray("destId");
      for (auto x : outputData.getDestId())
      {
        destId.add(x);
      }
      outputDoc["signalCode"] = outputData.getSignalCode();
      outputDoc["hopCount"] = outputData.getHopCount();
      outputDoc["contentName"] = outputData.getContentName();
      outputDoc["content"] = outputData.getContent();

      String returnstr;
      serializeJson(outputDoc, returnstr);
      return returnstr;
    }
    else if (signalCode == SIGNAL_DATA)
    {
      int hopCount = inputDoc["hopCount"];
      std::string contentName = inputDoc["contentName"] | "unknown";
      std::string content = inputDoc["content"] | "unknown";
      NodeInputData inputData = NodeInputData(senderId, destId, signalCode, hopCount, contentName, content);

      NodeOutputData outputData = (nodeInteractor.handleDataReceive(inputData));

      StaticJsonDocument<200> outputDoc;
      outputDoc["senderId"] = outputData.getSenderId();
      JsonArray destId = outputDoc.createNestedArray("destId");
      for (auto x : outputData.getDestId())
      {
        destId.add(x);
      }
      outputDoc["signalCode"] = outputData.getSignalCode();
      outputDoc["hopCount"] = outputData.getHopCount();
      outputDoc["contentName"] = outputData.getContentName();
      outputDoc["content"] = outputData.getContent();

      String returnstr;
      serializeJson(outputDoc, returnstr);
      return returnstr;
    }
  };

  void reciveSensorData(String msg)
  {
    StaticJsonDocument<200> inputDoc;
    DeserializationError error = deserializeJson(inputDoc, msg);
    if (error)
    {
      Serial.print("Deserialization failure: ");
      Serial.println(error.c_str());
      exit(0);
    }

    std::string senderId = "unknown";
    std::string destId = "unknown";
    std::string signalcode = "unknown";
    int hopCount = 0;
    std::string contentName = inputDoc["contentName"] | "unknown";
    std::string content = inputDoc["content"] | "unknown";
    NodeInputData inputData = NodeInputData(senderId, destId, signalcode, hopCount, contentName, content);

    nodeInteractor.handleSensorDataReceive(inputData);
  }
};

#endif // INCLUDED_Arduino_CONTROLLER_hpp_