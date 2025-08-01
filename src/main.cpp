#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Ticker.h>

#include <esp_now.h>
#include <WiFi.h>
#include "esp_wifi.h"

#include "config/Config.hpp"
#include "ESP-NOWControlData.hpp"
#include "ESP-NOWController.hpp"
#include "Sensor.h"
#include <TaskScheduler.h>

Scheduler userScheduler;

// SIGNAL
#define SIGNAL_INTEREST "INTEREST" // Interest
#define SIGNAL_DATA "DATA"         // Data

uint8_t myMacAddress[6]; // MACアドレスを格納する変数

/*********************< Needed for ESP-NOWController >**********************/

ESP_NOWController espNowController;

/*********************< Needed for Sensor >**********************/

// DHTTemperature sensorObj;

/// @brief Function to read sensor data
void readSensorData()
{
  // sensorObj.read();

  // doc["contentName"] = sensorObj.getContentName();
  // doc["content"] = sensorObj.getData();

  // 仮のセンサーデータ送信処理（実装に応じて変更）
  ESP_NOWControlData sensorData = {
      .txAddress = {}, // 空
      .signalCode = SIGNAL_DATA,
      .hopCount = 0,
      .contentName = "/iot/buildingA/room101/temp",
      .content = "26.5C"};

  espNowController.receiveSensorData(sensorData);
}
Task taskReadSensorData(TASK_SECOND * 10, TASK_FOREVER, &readSensorData);

/*********************< Needed for ESP-NOW >**********************/

CommunicationData pixel_settings = {};

ESP_NOWControlData inputData = {
    .txAddress = {},
    .signalCode = "",
    .hopCount = 0,
    .contentName = "",
    .content = ""};

esp_now_peer_info_t peerInfo; // Peer information for ESP-NOW

/// @brief Callback function to handle data send
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  if (status == ESP_NOW_SEND_SUCCESS)
    Serial.println("Data sent successfully");
  else
    Serial.println("Data send failed");
}

/// @brief Callback function to handle data receive
void onDataReceive(const uint8_t *mac_addr, const uint8_t *data, int len)
{
  if (len == sizeof(CommunicationData))
  {
    memcpy(&pixel_settings, data, len);
    inputData.signalCode = pixel_settings.signalCode;
    inputData.hopCount = pixel_settings.hopCount;
    inputData.contentName = pixel_settings.contentName;
    inputData.content = pixel_settings.content;
    ESP_NOWControlData outputData = espNowController.receiveMessage(myMacAddress, mac_addr, inputData);

    // Prepare output data to send back
    CommunicationData outputPacket;
    outputPacket.signalCode = outputData.signalCode;
    outputPacket.hopCount = outputData.hopCount;
    outputPacket.contentName = outputData.contentName;
    outputPacket.content = outputData.content;

    // Send output data back
    for (const auto &addr : outputData.txAddress)
    {
      memcpy(peerInfo.peer_addr, addr.data(), 6);
      peerInfo.ifidx = WIFI_IF_STA; // Use the station interface
      peerInfo.channel = 0;         // Use the current channel
      peerInfo.encrypt = false;     // No encryption for simplicity
      if (esp_now_add_peer(&peerInfo) != ESP_OK)
      {
        Serial.println("Failed to add peer");
        return;
      }

      esp_err_t result = esp_now_send(peerInfo.peer_addr, (uint8_t *)&outputPacket, sizeof(outputPacket));
      if (result != ESP_OK)
        Serial.println(" Failed to send ESP-NOW data");

      esp_now_del_peer(peerInfo.peer_addr); // 後始末
    }
  }
  else
  {
    Serial.println("Received data size mismatch");
    return;
  }
}

/*********************< setup and loop >**********************/

void setup()
{
  Serial.begin(115200);
  if (!loadSystemConfig("/config.json"))
  {
    Serial.println("Failed to load system config!");
    return;
  }

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("ESP-NOW initialization failed");
    return;
  }
  esp_wifi_get_mac(WIFI_IF_STA, myMacAddress);
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataReceive);

  // sensorObj.run();
  userScheduler.addTask(taskReadSensorData);
  taskReadSensorData.enable();
}

void loop()
{
  //  If a value is entered in the serial
  if (Serial.available() > 0)
  {
    // Read to the end of a line
    String msg;
    msg = Serial.readStringUntil('\n');
    // Serial.printf("Received from Serial, msg=%s\n", msg.c_str());
  }
}