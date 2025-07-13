#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "painlessMesh.h"
#include <ArduinoJSON.h>
#include <Ticker.h>
#include <ArduinoController.hpp>
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
DHTTemperature sensorObj;

// SIGNAL
#define SIGNAL_INTEREST "1" // Interest
#define SIGNAL_DATA "2"     // Data
#define SIGNAL_INVALID "3"  // Invalid message

// JSONDoc
StaticJsonDocument<200> doc;

/*********************< Callback classes and functions >**********************/

void msgReception(uint32_t to, String const &msg)
{
  String processedmsg = arduinoController.receiveMessage(to, msg);
  // Serial.printf("Processed msg=%s\n", processedmsg.c_str());

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

  Serial.printf("send msg=%s\n", processedmsg.c_str());
  doc.clear();
}

// Read Sensor Data
void readSensorData()
{
  sensorObj.read();

  doc["contentName"] = sensorObj.getContentName();
  doc["content"] = sensorObj.getData();
  doc["time"] = mesh.getNodeTime();

  String sensorData;
  serializeJson(doc, sensorData);
  doc.clear();
  arduinoController.reciveSensorData(sensorData);
}
Task taskReadSensorData(TASK_SECOND * 10, TASK_FOREVER, &readSensorData);

// Needed for painless library
void receivedCallback(uint32_t from, String &msg)
{
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
  msgReception(mesh.getNodeId(), msg);
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
  sensorObj.run();

  // mesh.setDebugMsgTypes(ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE); // all types on
  mesh.setDebugMsgTypes(ERROR | STARTUP); // set before init() so that you can see startup messages

  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  userScheduler.addTask(taskReadSensorData);
  taskReadSensorData.enable();

  arduinoController.setMesh(&mesh);
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

    msgReception(mesh.getNodeId(), msg);
  }
}