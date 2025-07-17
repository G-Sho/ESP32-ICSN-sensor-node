#pragma once

#include <ArduinoJSON.h>
#include "UseCaseInteractor.hpp"
#include "InputData.hpp"
#include "OutputData.hpp"
#include "message/SignalCode.hpp"

// JSON
// StaticJsonDocument<200> inputDoc;
// StaticJsonDocument<200> outputDoc;
JsonDocument inputDoc;
JsonDocument outputDoc;

/// @brief Parses JSON document to InputData structure
/// @details This function extracts the necessary fields from the JSON document and constructs an InputData object
/// @param doc The JSON document containing the data
/// @param to The destination ID
/// @return Returns an InputData object constructed from the JSON document
InputData parseJsonToInputData(JsonDocument &doc, uint32_t to)
{
  std::string senderId = doc["senderId"];
  std::string destId = std::to_string(to);
  std::string signalCode = doc["signalCode"];
  int hopCount = doc["hopCount"];
  std::string contentName = doc["contentName"];
  std::string content = doc["content"];
  uint32_t time = doc["time"];
  return InputData(senderId, {destId}, signalCode, hopCount, contentName, content, time);
};

/// @brief Parses OutputData to JSON document
/// @details This function converts the OutputData structure into a JSON document format
/// @param outputData The OutputData structure to be converted
/// @return Returns the OutputData structure after parsing to JSON
OutputData parseOutputDataToJson(const OutputData &outputData)
{
  outputDoc["senderId"] = String(outputData.senderId.c_str());
  JsonArray destIdArray = outputDoc["destId"].to<JsonArray>();
  for (const auto &id : outputData.destId)
    destIdArray.add(String(id.c_str()));
  outputDoc["signalCode"] = String(outputData.signalCode.c_str());
  outputDoc["hopCount"] = outputData.hopCount;
  outputDoc["contentName"] = String(outputData.contentName.c_str());
  outputDoc["content"] = String(outputData.content.c_str());
  outputDoc["time"] = outputData.time;
  return outputData;
};

/// @brief ArduinoController class
/// @details This class is responsible for handling the communication with the Arduino controller.
class ArduinoController
{
private:
  UseCaseInteractor useCaseInteractor;

public:
  void setMesh(painlessMesh *meshPtr)
  {
    useCaseInteractor.setMesh(meshPtr); // NodeInteractor へ伝播
  };

  String receiveMessage(uint32_t to, String msg)
  {
    DeserializationError error = deserializeJson(inputDoc, msg);
    if (error)
    {
      Serial.print("Deserialization failure : ");
      Serial.println(error.c_str());
      exit(0);
    }

    std::string signalCode = inputDoc["signalCode"].as<std::string>();
    if (fromString(signalCode) == SignalCode::INTEREST)
    {
      InputData inputData = parseJsonToInputData(inputDoc, to);
      OutputData outputData = useCaseInteractor.handleInterestReceive(inputData);
      inputDoc.clear();

      parseOutputDataToJson(outputData);
      String returnstr;
      serializeJson(outputDoc, returnstr);
      outputDoc.clear();
      return returnstr;
    }
    else if (fromString(signalCode) == SignalCode::DATA)
    {
      InputData inputData = parseJsonToInputData(inputDoc, to);
      OutputData outputData = useCaseInteractor.handleInterestReceive(inputData);
      inputDoc.clear();

      parseOutputDataToJson(outputData);
      String returnstr;
      serializeJson(outputDoc, returnstr);
      outputDoc.clear();
      return returnstr;
    }
    else if (fromString(signalCode) == SignalCode::INVALID)
    {
      outputDoc.clear();
      outputDoc["signalCode"] = "INVALID";
      String returnstr;
      serializeJson(outputDoc, returnstr);
      outputDoc.clear();
      return returnstr;
    }

    // Add a default return to handle unexpected cases
    outputDoc.clear();
    outputDoc["signalCode"] = "UNHANDLED";
    String returnstr;
    serializeJson(outputDoc, returnstr);
    outputDoc.clear();
    return returnstr;
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

    InputData inputData = parseJsonToInputData(inputDoc, 0);

    useCaseInteractor.handleSensorDataReceive(inputData);
    inputDoc.clear();
  }
};