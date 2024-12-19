#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "painlessMesh.h"
#include <ArduinoJSON.h>
#include <Ticker.h>
#include <ArduinoController.hpp>
#include "Sensor.h"

// MESH Details
#define MESH_PREFIX "RNTMESH"        // name for your MESH
#define MESH_PASSWORD "MESHpassword" // password for your MESH
#define MESH_PORT 5555               // default port

// タイマー制御用
Ticker ticker;
bool bReadyTicker = false;
const int iIntervalTime = 10; // 計測間隔（10秒）
void kickRoutine()
{
  bReadyTicker = true;
}

// Painless Mesh
painlessMesh mesh;
Scheduler userScheduler; // to control your personal task

// self-made
ArduinoController arduinoController;
DHTHumidity sensorObj;

// SIGNAL
#define SIGNAL_INTEREST "1" // Interest
#define SIGNAL_DATA "2"     // Data
#define SIGNAL_INVALID "3"  // Invalid message

/*********************< Callback classes and functions >**********************/

void msgReception(uint32_t to, String const &msg)
{
  String processedmsg = arduinoController.receiveMessage(to, msg);
  Serial.printf("Processed msg=%s\n", processedmsg.c_str());

  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, processedmsg);
  if (error)
  {
    Serial.print("Deserialization failure: ");
    Serial.println(error.c_str());
    exit(0);
  }

  String signalCode = doc["signalCode"];
  if (signalCode != SIGNAL_INVALID)
  {
    JsonArray destId = doc["destId"];
    for (JsonVariant value : destId)
    {
      if (value.as<String>() == "-1")
        mesh.sendBroadcast(processedmsg);
      else
        mesh.sendSingle((uint32_t)((value.as<String>()).toInt()), processedmsg);
    }
  }
}

// Read Sensor Data
void readSensorData()
{
  sensorObj.read();

  StaticJsonDocument<200> doc;
  doc["contentName"] = sensorObj.getContentName();
  doc["content"] = sensorObj.getData();

  String sensorData;
  serializeJson(doc, sensorData);
  arduinoController.reciveSensorData(sensorData);
}

// Needed for painless library
void receivedCallback(uint32_t from, String &msg)
{
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
  msgReception(mesh.getNodeId(), msg);
}

void newConnectionCallback(uint32_t nodeId)
{
  Serial.printf("New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback()
{
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset)
{
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

/*********************< setup and loop >**********************/

void setup()
{
  enableCore1WDT();
  Serial.begin(115200);
  sensorObj.run();

  // タイマー割り込みを開始する
  ticker.attach(iIntervalTime, kickRoutine);

  // mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes(ERROR | STARTUP); // set before init() so that you can see startup messages

  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  
  Serial.printf("%u\n", mesh.getNodeId());
}

void loop()
{
  // it will run the user scheduler as well
  mesh.update();

  // タイマー割り込みによってセンサデータ読み取りを実行する
  if (bReadyTicker)
  {
    readSensorData();
    bReadyTicker = false;
  }

  // シリアルに値が入力されているならその内容を送信
  if (Serial.available() > 0)
  {
    // シリアルデータの受信 (改行まで)
    String msg;
    msg = Serial.readStringUntil('\n');

    Serial.printf("Received from Serial msg=%s\n", msg.c_str());

    msgReception(mesh.getNodeId(), msg);
  }

  delay(1);
}