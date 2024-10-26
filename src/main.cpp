#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>
#include <Ticker.h>
#include <NodeController.hpp>
#include "Sensor.h"

// MESH Details
#define MESH_PREFIX "RNTMESH"        // name for your MESH
#define MESH_PASSWORD "MESHpassword" // password for your MESH
#define MESH_PORT 5555               // default port

// タイマー制御用
Ticker ticker;
bool bReadyTicker = false;
const int iIntervalTime = 20; // 計測間隔（10秒）
void kickRoutine()
{
  bReadyTicker = true;
}

unsigned long previousMillis = 0;
// timer
hw_timer_t *timer1 = NULL;

// Painless Mesh
painlessMesh mesh;
Scheduler userScheduler; // to control your personal task
uint32_t chipId;         // chipId of this node

// self-made
NodeController nodeController;
DHTTemperature sensorObj;

// SIGNAL
#define SIGNAL_INTEREST '1' // Interest
#define SIGNAL_DATA '2'     // Data
#define SIGANAL_INVALID '3' // Invalid message

/*********************< Callback classes and functions >**********************/

void msgReception(uint32_t to, String const &msg)
{  
  String processedmsg = nodeController.receiveMessage(to, msg);
  JSONVar myObject = JSON.parse(processedmsg.c_str());
  String senderId = JSON.stringify(myObject["senderId"]).substring(1, JSON.stringify(myObject["senderId"]).length() - 1); // chipId of sender
  String destId = JSON.stringify(myObject["destId"]).substring(1, JSON.stringify(myObject["destId"]).length() - 1);       // chipId of destination
  String signalCode = JSON.stringify(myObject["signalCode"]).substring(1, JSON.stringify(myObject["signalCode"]).length() - 1);

  if (signalCode[0] != SIGANAL_INVALID)
  {
    if (destId == "-1")
      mesh.sendBroadcast(processedmsg);
    else
      mesh.sendSingle((uint32_t)(destId.toInt()), processedmsg);
  }
}

// Read Sensor Data
void readSensorData()
{
  sensorObj.read();
  // JSON作って送る
  JSONVar jsonData;
  jsonData["destId"] = String((int)mesh.getNodeId());
  jsonData["senderId"] = String((int)mesh.getNodeId());
  jsonData["signalCode"] = SIGNAL_DATA;
  jsonData["hopCount"] = 0;
  jsonData["contentName"] = sensorObj.getContentName();
  jsonData["content"] = sensorObj.getData();
  String data = JSON.stringify(jsonData);
  nodeController.reciveSensorData(data);
}

// Needed for painless library
void receivedCallback(uint32_t from, String &msg)
{
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
  msgReception(chipId, msg);
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
  // timer
  timer1 = timerBegin(0, getApbFrequency()/1000000, true);
  timerStart(timer1);

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

  chipId = mesh.getNodeId();
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

    msgReception(chipId, msg);
  }
}