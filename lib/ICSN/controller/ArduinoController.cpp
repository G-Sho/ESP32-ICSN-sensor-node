#include "ArduinoController.hpp"
#include "message/SignalCode.hpp"

// JSON documents
StaticJsonDocument<512> inputDoc;
StaticJsonDocument<512> outputDoc;
// static JsonDocument inputDoc;
// static JsonDocument outputDoc;

// Helper: parse JSON to InputData
static InputData parseJsonToInputData(JsonDocument &doc, uint32_t to)
{
    std::string senderId = doc["senderId"].is<const char *>() ? doc["senderId"].as<const char *>() : "";
    std::string destId = std::to_string(to);
    std::string signalCode = doc["signalCode"].is<const char *>() ? doc["signalCode"].as<const char *>() : "";
    int hopCount = doc["hopCount"].is<int>() ? doc["hopCount"].as<int>() : 0;
    std::string contentName = doc["contentName"].is<const char *>() ? doc["contentName"].as<const char *>() : "";
    std::string content = doc["content"].is<const char *>() ? doc["content"].as<const char *>() : "";
    uint32_t time = doc["time"].is<uint32_t>() ? doc["time"].as<uint32_t>() : 0;

    return InputData(senderId, {destId}, signalCode, hopCount, contentName, content, time);
}

// Helper: convert OutputData to JSON
static void parseOutputDataToJson(const OutputData &outputData)
{
    outputDoc.clear();
    outputDoc["senderId"] = String(outputData.senderId.c_str());
    JsonArray destIdArray = outputDoc["destId"].to<JsonArray>();
    for (const auto &id : outputData.destId)
        destIdArray.add(String(id.c_str()));
    outputDoc["signalCode"] = String(outputData.signalCode.c_str());
    outputDoc["hopCount"] = outputData.hopCount;
    outputDoc["contentName"] = String(outputData.contentName.c_str());
    outputDoc["content"] = String(outputData.content.c_str());
    outputDoc["time"] = outputData.time;
}

// ArduinoController method definitions

void ArduinoController::setMesh(painlessMesh *meshPtr)
{
    if (meshPtr == nullptr)
    {
        Serial.println("Error: Mesh pointer is null.");
        return;
    }
    useCaseInteractor.setMesh(meshPtr);
}

String ArduinoController::receiveMessage(uint32_t to, String msg)
{
    DeserializationError error = deserializeJson(inputDoc, msg);
    if (error)
    {
        Serial.print("Deserialization failure: ");
        Serial.println(error.c_str());
        return "{\"signalCode\":\"INVALID\"}";
    }

    std::string signalCode = inputDoc["signalCode"].as<std::string>();
    SignalCode code = fromString(signalCode);

    if (code == SignalCode::INTEREST)
    {
        InputData inputData = parseJsonToInputData(inputDoc, to);
        OutputData outputData = useCaseInteractor.handleInterestReceive(inputData);
        inputDoc.clear();

        parseOutputDataToJson(outputData);
        String returnstr;
        serializeJson(outputDoc, returnstr);
        return returnstr;
    }
    else if (code == SignalCode::DATA)
    {
        InputData inputData = parseJsonToInputData(inputDoc, to);
        OutputData outputData = useCaseInteractor.handleDataReceive(inputData);
        inputDoc.clear();

        parseOutputDataToJson(outputData);
        String returnstr;
        serializeJson(outputDoc, returnstr);
        return returnstr;
    }
    else if (code == SignalCode::INVALID)
    {
        outputDoc.clear();
        outputDoc["signalCode"] = "INVALID";
        String returnstr;
        serializeJson(outputDoc, returnstr);
        return returnstr;
    }

    outputDoc.clear();
    outputDoc["signalCode"] = "UNHANDLED";
    String returnstr;
    serializeJson(outputDoc, returnstr);
    return returnstr;
}

void ArduinoController::reciveSensorData(String msg)
{
    DeserializationError error = deserializeJson(inputDoc, msg);
    if (error)
    {
        Serial.print("Deserialization failure: ");
        Serial.println(error.c_str());
        return;
    }

    InputData inputData = parseJsonToInputData(inputDoc, 0);
    useCaseInteractor.handleSensorDataReceive(inputData);
    inputDoc.clear();
}

#ifdef UNIT_TEST

void ArduinoController::mockAddToCS(const std::string &name, const std::string &content)
{
    useCaseInteractor.mockAddToCS(name, content);
}

void ArduinoController::mockAddToFIB(const std::string &name, uint32_t nextHop)
{
    useCaseInteractor.mockAddToFIB(name, nextHop);
}

void ArduinoController::mockAddToPIT(const std::string &name, uint32_t fromNode)
{
    useCaseInteractor.mockAddToPIT(name, fromNode);
}

String ArduinoController::mockReadSensor(const std::string &name, const std::string &content)
{
    OutputData output(
        "0",     // senderId
        {"-1"},  // destId
        "DATA",  // signalCode
        0,       // hopCount
        name,    // contentName
        content, // content
        millis() // time
    );

    parseOutputDataToJson(output);
    String json;
    serializeJson(outputDoc, json);
    return json;
}

#endif