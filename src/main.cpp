#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Ticker.h>
#include <esp_now.h>
#include <WiFi.h>
#include "esp_wifi.h"

// パフォーマンス測定を有効にする
#define PERFORMANCE_MEASURE

#include "config/Config.hpp"
#include "ESP-NOWControlData.hpp"
#include "ESP-NOWController.hpp"
#include "Sensor.h"
#include <TaskScheduler.h>
#include "performance/PerformanceStats.hpp"

// === 定数 ===
#define SIGNAL_INTEREST "INTEREST"
#define SIGNAL_DATA "DATA"

// ブロードキャスト定数
constexpr uint8_t BROADCAST_ADDRESS[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// === グローバル ===
Scheduler userScheduler;
ESP_NOWController espNowController;
uint8_t myMacAddress[6];
esp_now_peer_info_t peerInfo;

// パフォーマンス統計
PerformanceStats packetProcessStats;

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
  strncpy(packet.signalCode, data.signalCode, MAX_SIGNAL_CODE_LENGTH - 1);
  packet.signalCode[MAX_SIGNAL_CODE_LENGTH - 1] = '\0';
  packet.hopCount = data.hopCount;
  strncpy(packet.contentName, data.contentName, MAX_CONTENT_NAME_LENGTH - 1);
  packet.contentName[MAX_CONTENT_NAME_LENGTH - 1] = '\0';
  strncpy(packet.content, data.content, MAX_CONTENT_LENGTH - 1);
  packet.content[MAX_CONTENT_LENGTH - 1] = '\0';

  for (const auto &addr : data.txAddress) {
    if (std::all_of(addr.begin(), addr.end(), [](uint8_t b) { return b == 0; })) continue;

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
    
    // ブロードキャストアドレス以外は送信後に削除
    if (!isBroadcastAddress(addr)) {
      esp_now_del_peer(peerInfo.peer_addr);
    }
  }
}

// === センサデータ送信タスク ===
void readSensorData() {
  Serial.println("Reading sensor data...");

  ESP_NOWControlData sensorData = {};
  sensorData.hopCount = 1;
  strncpy(sensorData.signalCode, SIGNAL_DATA, MAX_SIGNAL_CODE_LENGTH - 1);
  sensorData.signalCode[MAX_SIGNAL_CODE_LENGTH - 1] = '\0';
  strncpy(sensorData.contentName, "/iot/buildingA/room101/temp", MAX_CONTENT_NAME_LENGTH - 1);
  sensorData.contentName[MAX_CONTENT_NAME_LENGTH - 1] = '\0';
  strncpy(sensorData.content, "26.5C", MAX_CONTENT_LENGTH - 1);
  sensorData.content[MAX_CONTENT_LENGTH - 1] = '\0';

  Serial.printf("Sensor: %s = %s\n", sensorData.contentName, sensorData.content);
  espNowController.receiveSensorData(sensorData);
}
Task taskReadSensorData(TASK_SECOND * 10, TASK_FOREVER, &readSensorData);

// === INTEREST送信タスク ===
void sendInterest() {
  Serial.println("Sending INTEREST...");

  ESP_NOWControlData interest = {};
  std::copy(std::begin(BROADCAST_ADDRESS), std::end(BROADCAST_ADDRESS), interest.txAddress[0].begin());
  strncpy(interest.signalCode, SIGNAL_INTEREST, MAX_SIGNAL_CODE_LENGTH - 1);
  interest.signalCode[MAX_SIGNAL_CODE_LENGTH - 1] = '\0';
  interest.hopCount = 1;
  strncpy(interest.contentName, "/iot/buildingA/room101/temp", MAX_CONTENT_NAME_LENGTH - 1);
  interest.contentName[MAX_CONTENT_NAME_LENGTH - 1] = '\0';
  strncpy(interest.content, "N/A", MAX_CONTENT_LENGTH - 1);
  interest.content[MAX_CONTENT_LENGTH - 1] = '\0';

  sendPacketToAddresses(interest);
}
Task taskTestInterestsent(TASK_SECOND * 10, TASK_FOREVER, &sendInterest);

// === ESP-NOW コールバック ===
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  // Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Data sent successfully" : "Data send failed");
}

void onDataReceive(const uint8_t *mac_addr, const uint8_t *data, int len) {
  // パケット処理時間測定開始
  MEASURE_START(packet_timer);
  
  if (len != sizeof(CommunicationData)) {
    Serial.println("Received data size mismatch");
    MEASURE_END(packet_timer, packetProcessStats);
    return;
  }

  CommunicationData receivedPacket;
  memcpy(&receivedPacket, data, sizeof(CommunicationData));

  // ESP_NOWControlData に変換
  ESP_NOWControlData inputData = {};
  strncpy(inputData.signalCode, receivedPacket.signalCode, MAX_SIGNAL_CODE_LENGTH);
  inputData.hopCount = receivedPacket.hopCount;
  strncpy(inputData.contentName, receivedPacket.contentName, MAX_CONTENT_NAME_LENGTH);
  strncpy(inputData.content, receivedPacket.content, MAX_CONTENT_LENGTH);
  std::copy(mac_addr, mac_addr + 6, inputData.txAddress[0].begin());

  ESP_NOWControlData outputData = espNowController.receiveMessage(myMacAddress, mac_addr, inputData);
  sendPacketToAddresses(outputData);
  
  // パケット処理時間測定終了
  MEASURE_END(packet_timer, packetProcessStats);
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
    } else if (msg == "perf_stats") {
      Serial.println("[CMD] perf_stats received");
      packetProcessStats.printStats("Individual Packet Processing");
    } else if (msg == "perf_reset") {
      Serial.println("[CMD] perf_reset received");
      packetProcessStats.reset();
      Serial.println("Performance statistics reset.");
    } else if (msg == "help") {
      Serial.println("=== Available Commands ===");
      Serial.println("  send_interest - Send INTEREST via ESP-NOW");
      Serial.println("  read_sensor   - Simulate sensor data send");
      Serial.println("  perf_stats    - Show performance statistics");
      Serial.println("  perf_reset    - Reset performance statistics");
      Serial.println("  help          - Show this help");
    } else {
      Serial.printf("[WARN] Unknown command: %s\n", msg.c_str());
      Serial.println("Type 'help' to see available commands.");
    }
  }
}
