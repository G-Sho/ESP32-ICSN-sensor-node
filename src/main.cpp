// パフォーマンス測定を有効にする
#define PERFORMANCE_MEASURE

#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "painlessMesh.h"
#include <ArduinoJSON.h>
#include <cstring>
#include <cstdlib>
#include <Ticker.h>
#include "config/Config.hpp"
#include "controller/ArduinoController.hpp"
#include "Sensor.h"
#include "performance/PerformanceStats.hpp"
#include "entity/message/DestinationId.hpp"

// === 定数 ===
#define MESH_PREFIX "ICSN"
#define MESH_PASSWORD "MyPassword"
#define MESH_PORT 5555

#define SIGNAL_INTEREST "INTEREST"
#define SIGNAL_DATA "DATA"
#define SIGNAL_INVALID "INVALID"

// === グローバル ===
painlessMesh mesh;
Scheduler userScheduler;
ArduinoController arduinoController;
constexpr size_t JSON_DOC_SIZE = 512;
PerformanceStats packetProcessStats;

// === ヘルパー ===
bool hasBroadcastDestination(JsonArrayConst destId)
{
  for (JsonVariantConst value : destId)
  {
    const char *destination = value.as<const char *>();
    if (destination != nullptr && strcmp(destination, DEST_BROADCAST) == 0)
    {
      return true;
    }
  }
  return false;
}

bool tryParseNodeId(JsonVariantConst value, uint32_t &nodeId)
{
  if (value.is<uint32_t>())
  {
    nodeId = value.as<uint32_t>();
    return true;
  }

  const char *destination = value.as<const char *>();
  if (destination == nullptr || destination[0] == '\0')
  {
    return false;
  }

  char *endPtr = nullptr;
  unsigned long parsedValue = strtoul(destination, &endPtr, 10);
  if (endPtr != nullptr && *endPtr == '\0')
  {
    nodeId = static_cast<uint32_t>(parsedValue);
    return true;
  }

  return false;
}

void sendMessage(uint32_t from, const String &msg, JsonArrayConst destId)
{
  if (hasBroadcastDestination(destId))
  {
    for (uint32_t nodeId : mesh.getNodeList())
    {
      if (nodeId == from)
      {
        continue;
      }

      mesh.sendSingle(nodeId, msg);
    }
  }
  else
  {
    for (JsonVariantConst value : destId)
    {
      uint32_t nodeId = 0;
      if (!tryParseNodeId(value, nodeId))
      {
        Serial.println("Skipping message send: invalid destination node ID in destId array.");
        continue;
      }

      mesh.sendSingle(nodeId, msg);
    }
  }
}

// === コールバック ===
void msgReception(uint32_t from, uint32_t to, String const &msg)
{
  // パケット処理時間測定開始
  MEASURE_START(packet_timer);

  String processedmsg = arduinoController.receiveMessage(to, msg);

  StaticJsonDocument<JSON_DOC_SIZE> doc;
  DeserializationError error = deserializeJson(doc, processedmsg);
  if (error)
  {
    Serial.print("Deserialization failure: ");
    Serial.println(error.c_str());
    MEASURE_END(packet_timer, packetProcessStats);
    return;
  }

  JsonVariantConst signalCodeVariant = doc["signalCode"];
  const char *signalCode = signalCodeVariant.as<const char *>();
  if (signalCode == nullptr)
  {
    Serial.println("Invalid message: signalCode is missing or not a string.");
    MEASURE_END(packet_timer, packetProcessStats);
    return;
  }

  if (strcmp(signalCode, SIGNAL_DATA) == 0 || strcmp(signalCode, SIGNAL_INTEREST) == 0)
  {
    JsonArrayConst destId = doc["destId"].as<JsonArrayConst>();
    if (destId.isNull())
    {
      Serial.println("Invalid message: destId is missing or not an array.");
      MEASURE_END(packet_timer, packetProcessStats);
      return;
    }

    sendMessage(from, processedmsg, destId);
  }
  else
  {
    Serial.println("Received message with unsupported signalCode. No forwarding performed.");
  }

  // パケット処理時間測定終了
  MEASURE_END(packet_timer, packetProcessStats);
}

// === INTEREST送信 ===
void sendInterest(uint32_t targetNodeId = 0)
{
  if (targetNodeId == 0)
  {
    Serial.println("Sending INTEREST (broadcast)...");
  }
  else
  {
    Serial.printf("Sending INTEREST to: %u\n", targetNodeId);
  }

  StaticJsonDocument<JSON_DOC_SIZE> doc;
  doc["senderId"] = String(mesh.getNodeId());
  doc["signalCode"] = SIGNAL_INTEREST;

  JsonArray destId = doc.createNestedArray("destId");
  if (targetNodeId == 0)
  {
    destId.add(DEST_BROADCAST);
  }
  else
  {
    destId.add(String(targetNodeId));
  }

  doc["contentName"] = "/iot/buildingA/room101/temp";
  doc["content"] = "N/A";
  doc["time"] = mesh.getNodeTime();

  String interestMsg;
  if (serializeJson(doc, interestMsg) == 0)
  {
    Serial.println("Failed to serialize INTEREST message.");
    return;
  }

  if (targetNodeId == 0)
  {
    mesh.sendBroadcast(interestMsg);
  }
  else
  {
    mesh.sendSingle(targetNodeId, interestMsg);
  }
}

// === INTEREST定期送信用 ===
uint32_t interestTargetNode = 0;

void periodicSendInterest()
{
  sendInterest(interestTargetNode);
}
Task taskSendInterest(TASK_SECOND * 30, TASK_FOREVER, &periodicSendInterest);

// === センサデータ送信 ===
void readSensorData()
{
  Serial.println("Reading sensor data...");

  StaticJsonDocument<JSON_DOC_SIZE> doc;
  doc["contentName"] = "/iot/buildingA/room101/temp";
  doc["content"] = "26.5C";
  doc["time"] = mesh.getNodeTime();

  String sensorData;
  if (serializeJson(doc, sensorData) == 0)
  {
    Serial.println("Failed to serialize sensor data.");
    return;
  }
  arduinoController.reciveSensorData(sensorData);

  Serial.printf("Sensor: %s = %s\n", "/iot/buildingA/room101/temp", "26.5C");
}
Task taskReadSensorData(TASK_SECOND * 10, TASK_FOREVER, &readSensorData);

// === painlessMeshコールバック ===
void receivedCallback(uint32_t from, String &msg)
{
  Serial.printf("Received from %u: %s\n", from, msg.c_str());
  msgReception(from, mesh.getNodeId(), msg);
}

void newConnectionCallback(uint32_t nodeId)
{
  // Serial.printf("New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback()
{
  // Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset)
{
  // Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

// === setup() ===
void setup()
{
  Serial.begin(115200);
  Serial.println("Starting setup...");

  if (!loadSystemConfig("/config.json"))
  {
    Serial.println("Failed to load system config!");
    return;
  }

  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  userScheduler.addTask(taskReadSensorData);
  // taskReadSensorData.enable();

  userScheduler.addTask(taskSendInterest);

  arduinoController.setMesh(&mesh);

  Serial.println(mesh.getNodeId());
  readSensorData();
  Serial.println("Setup complete.");
}

// === loop() ===
void loop()
{
  mesh.update();

  if (Serial.available() > 0)
  {
    String msg = Serial.readStringUntil('\n');
    msg.trim();

    if (msg == "send_interest")
    {
      Serial.println("[CMD] send_interest received - Starting periodic INTEREST broadcast (30s interval)");
      interestTargetNode = 0;
      sendInterest(interestTargetNode);  // 即座に1回送信
      taskSendInterest.enableDelayed(TASK_SECOND * 30);  // 30秒後から定期送信開始
    }
    else if (msg.startsWith("send_interest "))
    {
      uint32_t targetNode = msg.substring(14).toInt();
      Serial.printf("[CMD] send_interest %u received - Starting periodic INTEREST to node (30s interval)\n", targetNode);
      interestTargetNode = targetNode;
      sendInterest(interestTargetNode);  // 即座に1回送信
      taskSendInterest.enableDelayed(TASK_SECOND * 30);  // 30秒後から定期送信開始
    }
    else if (msg == "stop_interest")
    {
      Serial.println("[CMD] stop_interest received - Stopping periodic INTEREST");
      taskSendInterest.disable();
    }
    else if (msg == "read_sensor")
    {
      Serial.println("[CMD] read_sensor received");
      readSensorData();
    }
    else if (msg == "perf_stats")
    {
      Serial.println("[CMD] perf_stats received");
      packetProcessStats.printStats("Message Processing");
    }
    else if (msg == "perf_reset")
    {
      Serial.println("[CMD] perf_reset received");
      packetProcessStats.reset();
      Serial.println("Performance statistics reset.");
    }
    else if (msg == "help")
    {
      Serial.println("=== Available Commands ===");
      Serial.println("  send_interest        - Start periodic INTEREST broadcast (30s interval)");
      Serial.println("  send_interest <node> - Start periodic INTEREST to specific node (30s interval)");
      Serial.println("  stop_interest        - Stop periodic INTEREST sending");
      Serial.println("  read_sensor          - Simulate sensor data read");
      Serial.println("  perf_stats           - Show performance statistics");
      Serial.println("  perf_reset           - Reset performance statistics");
      Serial.println("  help                 - Show this help");
    }
    else
    {
      Serial.printf("[WARN] Unknown command: %s\n", msg.c_str());
      Serial.println("Type 'help' to see available commands.");
    }
  }
}
