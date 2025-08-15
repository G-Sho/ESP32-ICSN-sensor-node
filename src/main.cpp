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

// === 定数 ===
#define SIGNAL_INTEREST "INTEREST"
#define SIGNAL_DATA "DATA"

// === グローバル ===
Scheduler userScheduler;
ESP_NOWController espNowController;
uint8_t myMacAddress[6];
esp_now_peer_info_t peerInfo;

// === ヘルパー ===
bool isBroadcastAddress(const std::array<uint8_t, 6> &addr) {
  return std::all_of(addr.begin(), addr.end(), [](uint8_t b) { return b == 0xFF; });
}

void printMac(const uint8_t *mac) {
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", mac[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
}

void sendPacketToAddresses(const ESP_NOWControlData &data) {
  CommunicationData packet = {};
  strncpy(packet.signalCode, data.signalCode, MAX_SIGNAL_CODE_LENGTH);
  packet.hopCount = data.hopCount;
  strncpy(packet.contentName, data.contentName, MAX_CONTENT_NAME_LENGTH);
  strncpy(packet.content, data.content, MAX_CONTENT_LENGTH);

  for (const auto &addr : data.txAddress) {
    if (std::all_of(addr.begin(), addr.end(), [](uint8_t b) { return b == 0; })) continue;

    if (isBroadcastAddress(addr)) {
      uint8_t broadcastAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
      esp_now_send(broadcastAddress, (uint8_t *)&packet, sizeof(packet));
    } else {
      memcpy(peerInfo.peer_addr, addr.data(), 6);
      peerInfo.ifidx = WIFI_IF_STA;
      peerInfo.channel = 1;
      peerInfo.encrypt = false;

      if (!esp_now_is_peer_exist(peerInfo.peer_addr)) {
        if (esp_now_add_peer(&peerInfo) != ESP_OK) {
          Serial.println("Failed to add peer");
          continue;
        }
      }

      esp_now_send(peerInfo.peer_addr, (uint8_t *)&packet, sizeof(packet));
      esp_now_del_peer(peerInfo.peer_addr);
    }
  }
}

// === センサデータ送信タスク ===
void readSensorData() {
  Serial.println("Reading sensor data...");

  ESP_NOWControlData sensorData = {};
  sensorData.hopCount = 1;
  strncpy(sensorData.signalCode, SIGNAL_DATA, MAX_SIGNAL_CODE_LENGTH);
  strncpy(sensorData.contentName, "/iot/buildingA/room101/temp", MAX_CONTENT_NAME_LENGTH);
  strncpy(sensorData.content, "26.5C", MAX_CONTENT_LENGTH);

  Serial.printf("Sensor: %s = %s\n", sensorData.contentName, sensorData.content);
  espNowController.receiveSensorData(sensorData);
}
Task taskReadSensorData(TASK_SECOND * 10, TASK_FOREVER, &readSensorData);

// === INTEREST送信タスク ===
void sendInterest() {
  Serial.println("Sending INTEREST...");

  ESP_NOWControlData interest = {};
  interest.txAddress[0] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  strncpy(interest.signalCode, SIGNAL_INTEREST, MAX_SIGNAL_CODE_LENGTH);
  interest.hopCount = 1;
  strncpy(interest.contentName, "/iot/buildingA/room101/temp", MAX_CONTENT_NAME_LENGTH);
  strncpy(interest.content, "N/A", MAX_CONTENT_LENGTH);

  sendPacketToAddresses(interest);
}
Task taskTestInterestsent(TASK_SECOND * 10, TASK_FOREVER, &sendInterest);

// === ESP-NOW コールバック ===
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Data sent successfully" : "Data send failed");
}

void onDataReceive(const uint8_t *mac_addr, const uint8_t *data, int len) {
  if (len != sizeof(CommunicationData)) {
    Serial.println("Received data size mismatch");
    return;
  }

  CommunicationData receivedPacket;
  memcpy(&receivedPacket, data, sizeof(CommunicationData));
  
  // Ensure null termination for safety
  receivedPacket.signalCode[MAX_SIGNAL_CODE_LENGTH - 1] = '\0';
  receivedPacket.contentName[MAX_CONTENT_NAME_LENGTH - 1] = '\0';
  receivedPacket.content[MAX_CONTENT_LENGTH - 1] = '\0';

  Serial.print("Received from: ");
  printMac(mac_addr);
  Serial.printf("Signal Code: %s, Hop Count: %d, Content Name: %s, Content: %s\n",
                receivedPacket.signalCode,
                receivedPacket.hopCount,
                receivedPacket.contentName,
                receivedPacket.content);

  // ESP_NOWControlData に変換
  ESP_NOWControlData inputData = {};
  strncpy(inputData.signalCode, receivedPacket.signalCode, MAX_SIGNAL_CODE_LENGTH - 1);
  inputData.signalCode[MAX_SIGNAL_CODE_LENGTH - 1] = '\0';
  inputData.hopCount = receivedPacket.hopCount;
  strncpy(inputData.contentName, receivedPacket.contentName, MAX_CONTENT_NAME_LENGTH - 1);
  inputData.contentName[MAX_CONTENT_NAME_LENGTH - 1] = '\0';
  strncpy(inputData.content, receivedPacket.content, MAX_CONTENT_LENGTH - 1);
  inputData.content[MAX_CONTENT_LENGTH - 1] = '\0';
  
  // 送信者のMACアドレスを最初の要素に設定、他は0で初期化済み
  std::copy(mac_addr, mac_addr + 6, inputData.txAddress[0].begin());

  ESP_NOWControlData outputData = espNowController.receiveMessage(myMacAddress, mac_addr, inputData);
  sendPacketToAddresses(outputData);
}

// === ブロードキャストピア追加 ===
void addBroadcastPeer() {
  esp_now_peer_info_t info = {};
  memset(info.peer_addr, 0xFF, 6);
  info.channel = 1;
  info.ifidx = WIFI_IF_STA;
  info.encrypt = false;

  if (!esp_now_is_peer_exist(info.peer_addr)) {
    if (esp_now_add_peer(&info) == ESP_OK) {
      Serial.println("Broadcast peer added successfully");
    } else {
      Serial.println("Failed to add broadcast peer");
    }
  } else {
    Serial.println("Broadcast peer already exists");
  }
}

// === setup() ===
void setup() {
  Serial.begin(115200);
  Serial.println("Starting setup...");

  if (!loadSystemConfig("/config.json")) {
    Serial.println("Failed to load system config!");
    return;
  }

  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW initialization failed");
    return;
  }

  esp_wifi_get_mac(WIFI_IF_STA, myMacAddress);
  Serial.print("My MAC Address: ");
  printMac(myMacAddress);

  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataReceive);
  addBroadcastPeer();

  Serial.println("ESP-NOW initialized successfully");

  // タスクの有効化（必要に応じて）
  // userScheduler.addTask(taskReadSensorData); taskReadSensorData.enable();
  // userScheduler.addTask(taskTestInterestsent); taskTestInterestsent.enable();

  Serial.println("Setup complete.");
}

// === loop() ===
void loop() {
  userScheduler.execute();

  if (Serial.available() > 0) {
    String msg = Serial.readStringUntil('\n');
    msg.trim();

    if (msg == "send_interest") {
      Serial.println("[CMD] send_interest received");
      sendInterest();
    } else if (msg == "read_sensor") {
      Serial.println("[CMD] read_sensor received");
      readSensorData();
    } else if (msg == "help") {
      Serial.println("=== Available Commands ===");
      Serial.println("  send_interest - Send INTEREST via ESP-NOW");
      Serial.println("  read_sensor   - Simulate sensor data send");
      Serial.println("  help          - Show this help");
    } else {
      Serial.printf("[WARN] Unknown command: %s\n", msg.c_str());
      Serial.println("Type 'help' to see available commands.");
    }
  }
}
