// パフォーマンス測定を有効にする
#define PERFORMANCE_MEASURE

#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "painlessMesh.h"
#include <ArduinoJSON.h>
#include <Ticker.h>
#include "config/Config.hpp"
#include "controller/ArduinoController.hpp"
#include "Sensor.h"
#include "performance/PerformanceStats.hpp"

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
StaticJsonDocument<512> doc;
PerformanceStats packetProcessStats;

// === ヘルパー ===
bool hasBroadcastDestination(const JsonArray &destId)
{
  for (JsonVariant value : destId)
  {
    if (value.as<String>() == DEST_BROADCAST)
    {
      return true;
    }
  }
  return false;
}

void sendMessage(uint32_t from, const String &msg, const JsonArray &destId)
{
  if (hasBroadcastDestination(destId))
  {
    for (uint32_t nodeId : mesh.getNodeList())
    {
      if (from != mesh.getNodeId())
      {
        mesh.sendSingle(nodeId, msg);
      }
    }
  }
  else
  {
    for (JsonVariant value : destId)
    {
      mesh.sendSingle((uint32_t)((value.as<String>()).toInt()), msg);
    }
  }
}

// === コールバック ===
void msgReception(uint32_t from, uint32_t to, String const &msg)
{
  // パケット処理時間測定開始
  MEASURE_START(packet_timer);

  String processedmsg = arduinoController.receiveMessage(to, msg);

  DeserializationError error = deserializeJson(doc, processedmsg);
  if (error)
  {
    Serial.print("Deserialization failure: ");
    Serial.println(error.c_str());
    MEASURE_END(packet_timer, packetProcessStats);
    return;
  }

  String signalCode = doc["signalCode"];
  if (signalCode == SIGNAL_DATA || signalCode == SIGNAL_INTEREST)
  {
    JsonArray destId = doc["destId"];
    sendMessage(from, processedmsg, destId);
  }

  doc.clear();

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
  serializeJson(doc, interestMsg);
  doc.clear();

  if (targetNodeId == 0)
  {
    mesh.sendBroadcast(interestMsg);
  }
  else
  {
    mesh.sendSingle(targetNodeId, interestMsg);
  }
}

// === センサデータ送信 ===
void readSensorData()
{
  Serial.println("Reading sensor data...");

  doc["contentName"] = "/iot/buildingA/room101/temp";
  doc["content"] = "26.5C";
  doc["time"] = mesh.getNodeTime();

  String sensorData;
  serializeJson(doc, sensorData);
  doc.clear();
  arduinoController.reciveSensorData(sensorData);

  Serial.printf("Sensor: %s = %s\n", "/iot/buildingA/room101/temp", "26.5C");
}
Task taskReadSensorData(TASK_SECOND * 10, TASK_FOREVER, &readSensorData);

// === painlessMeshコールバック ===
void receivedCallback(uint32_t from, String &msg)
{
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

  arduinoController.setMesh(&mesh);

  Serial.println(mesh.getNodeId());
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
      Serial.println("[CMD] send_interest received");
      sendInterest();
    }
    else if (msg.startsWith("send_interest "))
    {
      uint32_t targetNode = msg.substring(14).toInt();
      Serial.printf("[CMD] send_interest %u received\n", targetNode);
      sendInterest(targetNode);
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
      Serial.println("  send_interest        - Send INTEREST (broadcast)");
      Serial.println("  send_interest <node> - Send INTEREST to specific node");
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
