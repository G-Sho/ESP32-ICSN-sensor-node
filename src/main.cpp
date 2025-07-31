#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Ticker.h>

#include <esp_now.h>
#include <WiFi.h>

#include "config/Config.hpp"
#include "controller/ArduinoController.hpp"
#include "Sensor.h"

// self-made
ArduinoController arduinoController;
// DHTTemperature sensorObj;

// SIGNAL
#define SIGNAL_INTEREST "INTEREST" // Interest
#define SIGNAL_DATA "DATA"         // Data

/*********************< functions >**********************/

// Read Sensor Data
void readSensorData()
{
  // sensorObj.read();

  // doc["contentName"] = sensorObj.getContentName();
  // doc["content"] = sensorObj.getData();

  // doc["contentName"] = "/iot/buildingA/room101/temp";
  // doc["content"] = "26.5C";
  // doc["time"] = mesh.getNodeTime();

  // String sensorData;
  // serializeJson(doc, sensorData);
  // doc.clear();
  // arduinoController.reciveSensorData(sensorData);

  // doc["contentName"] = "/iot/buildingA/room101/humidity";
  // doc["content"] = "55.2%";
  // doc["time"] = mesh.getNodeTime();
  // serializeJson(doc, sensorData);
  // doc.clear();
  // arduinoController.reciveSensorData(sensorData);

  // doc["contentName"] = "/iot/buildingA/room101/CO2";
  // doc["content"] = "438ppm";
  // doc["time"] = mesh.getNodeTime();
  // serializeJson(doc, sensorData);
  // doc.clear();
  // arduinoController.reciveSensorData(sensorData);

  // doc["contentName"] = "/iot/buildingA/room101/light";
  // doc["content"] = "123.4lux";
  // doc["time"] = mesh.getNodeTime();
  // serializeJson(doc, sensorData);
  // doc.clear();
  // arduinoController.reciveSensorData(sensorData);
}
Task taskReadSensorData(TASK_SECOND * 10, TASK_FOREVER, &readSensorData);

/*********************< Needed for ESP-NOW >**********************/

/// @brief It is used in the ESP-NOW communication
struct ControlData{
  String signalCode; // SIGNAL_INTEREST, SIGNAL_DATA
  uint8_t destId[6]; // Destination ID for ESP-NOW
  uint8_t fromId[6]; // From ID for ESP-NOW
  String contentName;
  String content;
};

/// @brief This variable is used to store data received via ESP-NOW
ControlData pixel_settings = {
  .signalCode = "",
  .contentName = "",
  .content = ""
};

esp_now_peer_info_t peerInfo; // Peer information for ESP-NOW

/// @brief Callback function to handle data send
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if(status == ESP_NOW_SEND_SUCCESS) {
    Serial.println("Data sent successfully");
  } else {
    Serial.println("Data send failed");
  }
}

/// @brief Callback function to handle data receive
void onDataReceive(const uint8_t *mac_addr, const uint8_t *data, int len) {
  ControlData ControlData;
  if(len == sizeof(ControlData)){
    memcpy(&pixel_settings, data, len);
  }else{
    Serial.println("Received data size mismatch");
    return;
  }
}

/*********************< setup and loop >**********************/

void setup()
{
  Serial.begin(115200);
  if (!loadSystemConfig("/config.json")){
    Serial.println("Failed to load system config!");
    return;
  }

  WiFi.mode(WIFI_STA);
  if(esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW initialization failed");
    return;
  }
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataReceive);

  // sensorObj.run();
  userScheduler.addTask(taskReadSensorData);
  taskReadSensorData.enable();

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

  }
}