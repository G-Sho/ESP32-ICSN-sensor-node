#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Ticker.h>
#include <esp_now.h>
#include <WiFi.h>
#include "esp_wifi.h"

// パフォーマンス測定を有効にする
#define PERFORMANCE_MEASURE

#include "config/Config.hpp"
#include "ESP-NOWControlData.hpp"
#include "ESP-NOWController.hpp"
#include "PeerCounterManager.hpp"
#include "Sensor.h"
#include "performance/PerformanceStats.hpp"

// === 定数 ===
#define SIGNAL_INTEREST "INTEREST"
#define SIGNAL_DATA "DATA"

// ブロードキャスト定数
constexpr uint8_t BROADCAST_ADDRESS[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// テスト用MACアドレス
constexpr uint8_t TEST_MAC_A[6] = {0xCC, 0x7B, 0x5C, 0x9A, 0xF3, 0xC4};
constexpr uint8_t TEST_MAC_B[6] = {0xCC, 0x7B, 0x5C, 0x9A, 0xF3, 0xAC};

// === グローバル ===
ESP_NOWController espNowController;
PeerCounterManager peerCounterManager;
uint8_t myMacAddress[6];
esp_now_peer_info_t peerInfo;

// パフォーマンス統計
PerformanceStats packetProcessStats;

// === タイマー関連 ===
constexpr float SENSOR_INTERVAL_SEC = 10.0f;
constexpr float INTEREST_INTERVAL_SEC = 10.0f;
constexpr float AUTO_INTEREST_DELAY_SEC = 40.0f;
constexpr bool AUTO_SENSOR_ENABLED = true; // 起動後の自動センサデータ読み取りを有効にするかどうか
constexpr bool AUTO_INTEREST_ENABLED = false; // 起動後の自動INTEREST送信を有効にするかどうか
constexpr uint32_t LOOP_IDLE_DELAY_MS = 5;  // Allow IDLE task scheduling & reduce active time

Ticker sensorTicker;
Ticker interestTicker;
Ticker autoInterestTicker;

volatile bool sensorReadRequested = false;
volatile bool interestSendRequested = false;
volatile bool autoInterestStartRequested = false;

void IRAM_ATTR onSensorTicker() { sensorReadRequested = true; }
void IRAM_ATTR onInterestTicker() { interestSendRequested = true; }
void IRAM_ATTR onAutoInterestTicker() { autoInterestStartRequested = true; }

void cancelAutoInterestStart() {
  autoInterestTicker.detach();
  autoInterestStartRequested = false;
}

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

    // 送信カウンタをインクリメントしてパケットに設定
    bool counterSuccess = false;
    packet.counter = peerCounterManager.incrementTxCounter(addr.data(), counterSuccess);
    if (!counterSuccess) {
      continue;
    }

    memcpy(peerInfo.peer_addr, addr.data(), 6);
    peerInfo.ifidx = WIFI_IF_STA;
    peerInfo.channel = 1;

    // 暗号化設定
    if (systemConfig.encryptionEnabled && !isBroadcastAddress(addr)) {
      peerInfo.encrypt = true;
      memcpy(peerInfo.lmk, systemConfig.lmk, ESP_NOW_LMK_LEN);
    } else {
      peerInfo.encrypt = false;
    }

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
  // Serial.println("Reading sensor data...");

  ESP_NOWControlData sensorData = {};
  sensorData.hopCount = 1;
  strncpy(sensorData.signalCode, SIGNAL_DATA, MAX_SIGNAL_CODE_LENGTH - 1);
  sensorData.signalCode[MAX_SIGNAL_CODE_LENGTH - 1] = '\0';
  strncpy(sensorData.contentName, "/iot/buildingA/room101/temp", MAX_CONTENT_NAME_LENGTH - 1);
  sensorData.contentName[MAX_CONTENT_NAME_LENGTH - 1] = '\0';
  strncpy(sensorData.content, "26.5C", MAX_CONTENT_LENGTH - 1);
  sensorData.content[MAX_CONTENT_LENGTH - 1] = '\0';

  // Serial.printf("Sensor: %s = %s\n", sensorData.contentName, sensorData.content);
  espNowController.receiveSensorData(sensorData);
}

// === INTEREST送信 ===
void sendInterest(const uint8_t* targetMac = nullptr) {
  if (targetMac == nullptr) {
    Serial.println("Sending INTEREST (broadcast)...");
  } else {
    Serial.print("Sending INTEREST to: ");
    printMac(targetMac);
  }

  ESP_NOWControlData interest = {};
  if (targetMac == nullptr) {
    std::copy(std::begin(BROADCAST_ADDRESS), std::end(BROADCAST_ADDRESS), interest.txAddress[0].begin());
  } else {
    std::copy(targetMac, targetMac + 6, interest.txAddress[0].begin());
  }
  strncpy(interest.signalCode, SIGNAL_INTEREST, MAX_SIGNAL_CODE_LENGTH - 1);
  interest.signalCode[MAX_SIGNAL_CODE_LENGTH - 1] = '\0';
  interest.hopCount = 1;
  strncpy(interest.contentName, "/iot/buildingA/room101/temp", MAX_CONTENT_NAME_LENGTH - 1);
  interest.contentName[MAX_CONTENT_NAME_LENGTH - 1] = '\0';
  strncpy(interest.content, "N/A", MAX_CONTENT_LENGTH - 1);
  interest.content[MAX_CONTENT_LENGTH - 1] = '\0';

  sendPacketToAddresses(interest);
}

// === INTEREST定期送信用 ===
const uint8_t* interestTargetMac = nullptr;

void periodicSendInterest() { sendInterest(interestTargetMac); }

void startInterestTicker() {
  interestTicker.detach();
  interestTicker.attach(INTEREST_INTERVAL_SEC, onInterestTicker);
  interestSendRequested = false;
}

void stopInterestTicker() {
  interestTicker.detach();
  interestSendRequested = false;
}

// === 起動後の自動INTEREST送信 ===
void autoStartInterest() {
  cancelAutoInterestStart();
  Serial.println("[AUTO] Starting periodic INTEREST broadcast (10s interval)");
  interestTargetMac = nullptr;
  sendInterest(interestTargetMac);                    // 即座に1回送信
  startInterestTicker();                              // 10秒後から定期送信開始
}

// === ESP-NOW コールバック ===
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  // Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Data sent successfully" : "Data send failed");
}

void onDataReceive(const uint8_t *mac_addr, const uint8_t *data, int len) {
  Serial.print("Received packet from: ");
  printMac(mac_addr);
  
  // パケット処理時間測定開始
  MEASURE_START(packet_timer);
  
  if (len != sizeof(CommunicationData)) {
    Serial.println("Received data size mismatch");
    MEASURE_END(packet_timer, packetProcessStats);
    return;
  }

  CommunicationData receivedPacket;
  memcpy(&receivedPacket, data, sizeof(CommunicationData));

  // ブロードキャストかユニキャストかを判定
  std::array<uint8_t, 6> macArray;
  std::copy(mac_addr, mac_addr + 6, macArray.begin());
  bool isBroadcast = isBroadcastAddress(macArray);

  // ユニキャストの場合のみカウンタ検証を行う
  if (!isBroadcast) {
    if (!peerCounterManager.validateRxCounter(mac_addr, receivedPacket.counter)) {
      Serial.printf("[SECURITY] Replay attack detected! MAC: ");
      printMac(mac_addr);
      Serial.printf("[SECURITY] Expected rx_counter+1, got counter=%u\n", receivedPacket.counter);
      MEASURE_END(packet_timer, packetProcessStats);
      return;
    }
  }

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

  // PMKの設定（暗号化が有効な場合）
  if (systemConfig.encryptionEnabled) {
    if (esp_now_set_pmk(systemConfig.pmk) != ESP_OK) {
      Serial.println("Failed to set PMK");
      return;
    }
    Serial.println("ESP-NOW encryption enabled (PMK/LMK configured)");
  }

  esp_wifi_get_mac(WIFI_IF_STA, myMacAddress);
  Serial.print("My MAC Address: ");
  printMac(myMacAddress);

  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataReceive);

  Serial.println("ESP-NOW initialized successfully");

  if (AUTO_SENSOR_ENABLED) {
    sensorTicker.attach(SENSOR_INTERVAL_SEC, onSensorTicker);
    sensorReadRequested = true;  // 起動直後にも1回実行
  } else {
    Serial.println("[AUTO] Auto sensor read disabled");
  }

  if (AUTO_INTEREST_ENABLED) {
    Serial.println("[AUTO] Scheduling INTEREST broadcast to start in 40s");
    autoInterestTicker.once(AUTO_INTEREST_DELAY_SEC, onAutoInterestTicker);
  } else {
    Serial.println("[AUTO] Auto INTEREST start disabled");
  }

  Serial.println("Setup complete.");
}

// === loop() ===
void loop() {
  if (autoInterestStartRequested) {
    autoInterestStartRequested = false;
    autoStartInterest();
  }

  if (sensorReadRequested) {
    sensorReadRequested = false;
    readSensorData();
  }

  if (interestSendRequested) {
    interestSendRequested = false;
    periodicSendInterest();
  }

  if (Serial.available() > 0) {
    String msg = Serial.readStringUntil('\n');
    msg.trim();

    if (msg == "send_interest") {
      Serial.println("[CMD] send_interest received - Starting periodic INTEREST broadcast (10s interval)");
      cancelAutoInterestStart();
      interestTargetMac = nullptr;
      sendInterest(interestTargetMac);                    // 即座に1回送信
      startInterestTicker();                              // 10秒後から定期送信開始
    } else if (msg == "send_interest_a") {
      Serial.println("[CMD] send_interest_a received - Starting periodic INTEREST to MAC A (10s interval)");
      cancelAutoInterestStart();
      interestTargetMac = TEST_MAC_A;
      sendInterest(interestTargetMac);                    // 即座に1回送信
      startInterestTicker();                              // 10秒後から定期送信開始
    } else if (msg == "send_interest_b") {
      Serial.println("[CMD] send_interest_b received - Starting periodic INTEREST to MAC B (10s interval)");
      cancelAutoInterestStart();
      interestTargetMac = TEST_MAC_B;
      sendInterest(interestTargetMac);                    // 即座に1回送信
      startInterestTicker();                              // 10秒後から定期送信開始
    } else if (msg == "stop_interest") {
      Serial.println("[CMD] stop_interest received - Stopping periodic INTEREST");
      stopInterestTicker();
      cancelAutoInterestStart();
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
      Serial.println("  send_interest   - Start periodic INTEREST broadcast (10s interval)");
      Serial.println("  send_interest_a - Start periodic INTEREST to MAC A (10s interval)");
      Serial.println("  send_interest_b - Start periodic INTEREST to MAC B (10s interval)");
      Serial.println("  stop_interest   - Stop periodic INTEREST sending");
      Serial.println("  read_sensor     - Simulate sensor data send");
      Serial.println("  perf_stats      - Show performance statistics");
      Serial.println("  perf_reset      - Reset performance statistics");
      Serial.println("  help            - Show this help");
    } else {
      Serial.printf("[WARN] Unknown command: %s\n", msg.c_str());
      Serial.println("Type 'help' to see available commands.");
    }
  }

  // Allow other RTOS tasks and the IDLE task to run, while hardware timers keep firing.
  delay(LOOP_IDLE_DELAY_MS);
}
