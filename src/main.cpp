#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "painlessMesh.h"
#include <ArduinoJSON.h>
#include <Ticker.h>
#include "config/Config.hpp"
#include "controller/ArduinoController.hpp"
#include "Sensor.h"

// MESH Details
#define MESH_PREFIX "ICSN"         // name for your MESH
#define MESH_PASSWORD "MyPassword" // password for your MESH
#define MESH_PORT 5555             // default port

// Painless Mesh
painlessMesh mesh;
Scheduler userScheduler; // to control your personal task

// self-made
ArduinoController arduinoController;
// DHTTemperature sensorObj;

// SIGNAL
#define SIGNAL_INTEREST "INTEREST" // Interest
#define SIGNAL_DATA "DATA"         // Data
#define SIGNAL_INVALID "INVALID"   // Invalid message

// JSONDoc
StaticJsonDocument<512> doc;
// JsonDocument doc;

/*********************< Test >**********************/
int sendCnt = 0;
int resCnt = 0;
#define TASK_DURATION_SEC 60 // 1分

// 送信間隔（ミリ秒）を定数で定義し、変更可能にする
int sendIntervalMs = 10; // デフォルト10ms
int sendCountLimit = TASK_DURATION_SEC * 1000 / sendIntervalMs;

// 各タスクのインスタンス生成時にsendIntervalMsを使用
Task taskTestInterestTemp(TASK_MILLISECOND * sendIntervalMs, TASK_FOREVER, nullptr);
Task taskTestInterestHumidity(TASK_MILLISECOND * sendIntervalMs, TASK_FOREVER, nullptr);
Task taskTestInterestCo2(TASK_MILLISECOND * sendIntervalMs, TASK_FOREVER, nullptr);
Task taskTestInterestLight(TASK_MILLISECOND * sendIntervalMs, TASK_FOREVER, nullptr);

void testInterestTemp()
{
  String msg = "{\"senderId\":\"533722253\",\"signalCode\":\"INTEREST\",\"destId\":[\"1553658797\"],\"contentName\":\"/iot/buildingA/room101/temp\",\"content\":\"N/A\",\"time\":0}";
  mesh.sendSingle(1553658797, msg);
  sendCnt++;

  if (sendCnt >= sendCountLimit)
  {
    Serial.println("Send Interest Temp Task finished.");
    taskTestInterestTemp.disable(); // Disable the task after sending the required number of messages
  }
}
// taskTestInterestTemp.setCallback(&testInterestTemp);

void testInterestHumidity()
{
  String msg = "{\"senderId\":\"533722253\",\"signalCode\":\"INTEREST\",\"destId\":[\"1553658797\"],\"contentName\":\"/iot/buildingA/room101/humidity\",\"content\":\"N/A\",\"time\":0}";
  mesh.sendSingle(1553658797, msg);
  sendCnt++;

  if (sendCnt >= sendCountLimit)
  {
    Serial.println("Send Interest Humidity Task finished.");
    taskTestInterestHumidity.disable(); // Disable the task after sending the required number of messages
  }
}
// taskTestInterestHumidity.setCallback(&testInterestHumidity);

void testInterestCo2()
{
  String msg = "{\"senderId\":\"533722253\",\"signalCode\":\"INTEREST\",\"destId\":[\"1553658797\"],\"contentName\":\"/iot/buildingA/room101/CO2\",\"content\":\"N/A\",\"time\":0}";
  mesh.sendSingle(1553658797, msg);
  sendCnt++;

  if (sendCnt >= sendCountLimit)
  {
    Serial.println("Send Interest Co2 Task finished.");
    taskTestInterestCo2.disable(); // Disable the task after sending the required number of messages
  }
}
// taskTestInterestCo2.setCallback(&testInterestCo2);

void testInterestLight()
{
  String msg = "{\"senderId\":\"533722253\",\"signalCode\":\"INTEREST\",\"destId\":[\"1553658797\"],\"contentName\":\"/iot/buildingA/room101/light\",\"content\":\"N/A\",\"time\":0}";
  mesh.sendSingle(1553658797, msg);
  sendCnt++;

  if (sendCnt >= sendCountLimit)
  {
    Serial.println("Send Interest Light Task finished.");
    taskTestInterestLight.disable(); // Disable the task after sending the required number of messages
  }
}
// taskTestInterestLight.setCallback(&testInterestLight);

/*********************< Callback classes and functions >**********************/

void msgReception(uint32_t from, uint32_t to, String const &msg)
{
  String processedmsg = arduinoController.receiveMessage(to, msg);
  // Serial.printf("Processed msg=%s\n", processedmsg.c_str());

  DeserializationError error = deserializeJson(doc, processedmsg);
  if (error)
  {
    Serial.print("Deserialization failure: ");
    Serial.println(error.c_str());
    return;
  }

  String signalCode = doc["signalCode"];
  if (signalCode == SIGNAL_DATA || signalCode == SIGNAL_INTEREST)
  {
    JsonArray destId = doc["destId"];

    bool hasBroadcast = false;
    for (JsonVariant value : destId)
    {
      if (value.as<String>() == DEST_BROADCAST)
      {
        hasBroadcast = true;
        break;
      }
    }
    if (hasBroadcast)
    {
      for (uint32_t nodeId : mesh.getNodeList())
      {
        if (from != mesh.getNodeId())
        {
          mesh.sendSingle(nodeId, processedmsg);
        }
      }
    }
    else
    {
      for (JsonVariant value : destId)
      {
        mesh.sendSingle((uint32_t)((value.as<String>()).toInt()), processedmsg);
      }
    }
  }

  // Serial.printf("send msg=%s\n", processedmsg.c_str());
  doc.clear();
  resCnt++;
}

// Read Sensor Data
void readSensorData()
{
  // sensorObj.read();

  // doc["contentName"] = sensorObj.getContentName();
  // doc["content"] = sensorObj.getData();

  doc["contentName"] = "/iot/buildingA/room101/temp";
  doc["content"] = "26.5C";
  doc["time"] = mesh.getNodeTime();

  String sensorData;
  serializeJson(doc, sensorData);
  doc.clear();
  arduinoController.reciveSensorData(sensorData);

  doc["contentName"] = "/iot/buildingA/room101/humidity";
  doc["content"] = "55.2%";
  doc["time"] = mesh.getNodeTime();
  serializeJson(doc, sensorData);
  doc.clear();
  arduinoController.reciveSensorData(sensorData);

  doc["contentName"] = "/iot/buildingA/room101/CO2";
  doc["content"] = "438ppm";
  doc["time"] = mesh.getNodeTime();
  serializeJson(doc, sensorData);
  doc.clear();
  arduinoController.reciveSensorData(sensorData);

  doc["contentName"] = "/iot/buildingA/room101/light";
  doc["content"] = "123.4lux";
  doc["time"] = mesh.getNodeTime();
  serializeJson(doc, sensorData);
  doc.clear();
  arduinoController.reciveSensorData(sensorData);
}
Task taskReadSensorData(TASK_SECOND * 10, TASK_FOREVER, &readSensorData);

/*********************< Needed for painless library >**********************/

void receivedCallback(uint32_t from, String &msg)
{
  // Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
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

/*********************< setup and loop >**********************/

void setup()
{
  Serial.begin(115200);
  // sensorObj.run();
  if (!loadSystemConfig("/config.json"))
    Serial.println("Failed to load system config!");
  else
    Serial.println("System config loaded successfully.");

  // mesh.setDebugMsgTypes(ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE); // all types on
  mesh.setDebugMsgTypes(ERROR | STARTUP); // set before init() so that you can see startup messages

  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  userScheduler.addTask(taskReadSensorData);
  userScheduler.addTask(taskTestInterestTemp);
  userScheduler.addTask(taskTestInterestHumidity);
  userScheduler.addTask(taskTestInterestCo2);
  userScheduler.addTask(taskTestInterestLight);

  // Set callbacks for tasks
  taskTestInterestTemp.setCallback(&testInterestTemp);
  taskTestInterestHumidity.setCallback(&testInterestHumidity);
  taskTestInterestCo2.setCallback(&testInterestCo2);
  taskTestInterestLight.setCallback(&testInterestLight);

  taskReadSensorData.enable();

  arduinoController.setMesh(&mesh);

  // タスクのインターバルを初期値でセット
  taskTestInterestTemp.setInterval(TASK_MILLISECOND * sendIntervalMs);
  taskTestInterestHumidity.setInterval(TASK_MILLISECOND * sendIntervalMs);
  taskTestInterestCo2.setInterval(TASK_MILLISECOND * sendIntervalMs);
  taskTestInterestLight.setInterval(TASK_MILLISECOND * sendIntervalMs);
}

void loop()
{
  // It will run the user scheduler as well
  mesh.update();

  //  If a value is entered in the serial
  if (Serial.available() > 0)
  {
    // Read to the end of a line
    String msg;
    msg = Serial.readStringUntil('\n');
    // Serial.printf("Received from Serial, msg=%s\n", msg.c_str());

    if (msg == "getNodeList")
    {
      Serial.println("Node List:");
      for (uint32_t nodeId : mesh.getNodeList())
      {
        Serial.printf("Node ID: %u\n", nodeId);
      }
      return;
    }
    else if (msg == "subConnectionJson")
    {
      String res = mesh.subConnectionJson(true);
      Serial.printf("Sub Connection JSON: %s\n", res.c_str());
      return;
    }
    else if (msg == "StartTestInterestTemp")
    {
      taskTestInterestTemp.enable();
    }
    else if (msg == "StartTestInterestHumidity")
    {
      taskTestInterestHumidity.enable();
    }
    else if (msg == "StartTestInterestCo2")
    {
      taskTestInterestCo2.enable();
    }
    else if (msg == "StartTestInterestLight")
    {
      taskTestInterestLight.enable();
    }
    else if (msg == "result")
    {
      Serial.printf("sendCnt=%d, resCnt=%d\n", sendCnt, resCnt);
    }
    else if (msg.startsWith("setInterval="))
    {
      // 例: setInterval=50 で50msに変更
      int newInterval = msg.substring(12).toInt();
      if (newInterval > 0)
      {
        sendIntervalMs = newInterval;
        sendCountLimit = TASK_DURATION_SEC * 1000 / sendIntervalMs;

        // 各タスクのインターバルを更新
        taskTestInterestTemp.setInterval(TASK_MILLISECOND * sendIntervalMs);
        taskTestInterestHumidity.setInterval(TASK_MILLISECOND * sendIntervalMs);
        taskTestInterestCo2.setInterval(TASK_MILLISECOND * sendIntervalMs);
        taskTestInterestLight.setInterval(TASK_MILLISECOND * sendIntervalMs);

        Serial.printf("Send interval updated: %d ms\n", sendIntervalMs);
      }
      else
      {
        Serial.println("Invalid interval value.");
      }
      return;
    }
    else
    {
      msgReception(0, mesh.getNodeId(), msg);
    }
  }
}