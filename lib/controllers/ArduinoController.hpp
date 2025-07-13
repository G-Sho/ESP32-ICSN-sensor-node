#ifndef INCLUDED_Arduino_CONTROLLER_hpp_
#define INCLUDED_Arduino_CONTROLLER_hpp_

#include <ArduinoJSON.h>
#include "node/NodeInteractor.hpp"
#include "node/NodeInputData.hpp"
#include "node/NodeOutputData.hpp"

// SIGNAL
#define SIGNAL_INTEREST "1" // Interest
#define SIGNAL_DATA "2"     // Data
#define SIGANAL_INVALID "3" // Invalid message

// JSON Docs
StaticJsonDocument<200> inputDoc;
StaticJsonDocument<200> outputDoc;

class ArduinoController
{
private:
  NodeInteractor nodeInteractor;

public:
  void setMesh(painlessMesh *meshPtr)
  {
    nodeInteractor.setMesh(meshPtr); // NodeInteractor へ伝播
  };

  String receiveMessage(uint32_t to, String msg)
  {
    // Serial.printf("in receiveMessage, %s\n", msg.c_str());
    DeserializationError error = deserializeJson(inputDoc, msg);
    if (error)
    {
      Serial.print("Deserialization failure : ");
      Serial.println(error.c_str());
      exit(0);
    }

    std::string senderId = inputDoc["senderId"] | "unknown";
    std::string destId = std::to_string(to);
    std::string signalCode = inputDoc["signalCode"] | "unknown";

    if (signalCode == SIGNAL_INTEREST)
    {
      int hopCount = inputDoc["hopCount"] | -1;
      std::string contentName = inputDoc["contentName"] | "unknown";
      std::string content = inputDoc["content"] | "unknown";
      uint32_t time = inputDoc["time"];
      NodeInputData inputData = NodeInputData(senderId, destId, signalCode, hopCount, contentName, content, time);

      NodeOutputData outputData = (nodeInteractor.handleInterestReceive(inputData));
      inputDoc.clear();

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
      outputDoc.clear();
      return returnstr;
    }
    else if (signalCode == SIGNAL_DATA)
    {
      int hopCount = inputDoc["hopCount"] | -1;
      std::string contentName = inputDoc["contentName"] | "unknown";
      std::string content = inputDoc["content"] | "unknown";
      uint32_t time = inputDoc["time"];
      NodeInputData inputData = NodeInputData(senderId, destId, signalCode, hopCount, contentName, content, time);

      NodeOutputData outputData = (nodeInteractor.handleDataReceive(inputData));
      inputDoc.clear();

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
      outputDoc.clear();
      return returnstr;
    }
  };

  void reciveSensorData(String msg)
  {
    DeserializationError error = deserializeJson(inputDoc, msg);
    if (error)
    {
      Serial.print("Deserialization failure: ");
      Serial.println(error.c_str());
      exit(0);
    }

    std::string senderId = "unknown";
    std::string destId = "unknown";
    std::string signalCode = "unknown";
    int hopCount = -1;
    std::string contentName = inputDoc["contentName"] | "unknown";
    std::string content = inputDoc["content"] | "unknown";
    uint32_t time = inputDoc["time"];
    NodeInputData inputData = NodeInputData(senderId, destId, signalCode, hopCount, contentName, content, time);

    nodeInteractor.handleSensorDataReceive(inputData);
    inputDoc.clear();
  }
};

#endif // INCLUDED_Arduino_CONTROLLER_hpp_