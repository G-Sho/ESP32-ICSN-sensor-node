#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Ticker.h>
#include <esp_now.h>
#include <WiFi.h>
#include "esp_wifi.h"

#include "BuildProfile.hpp"
#include "ESP-NOWController.hpp"
#include "UseCaseInteractor.hpp"
#include "LRUForwardingInformationBase.hpp"
#include "LRUPendingInterestTable.hpp"
#include "LRUContentStore.hpp"
#include "Sensor.h"

// === グローバル ===
LRUForwardingInformationBase fibRepository;
LRUPendingInterestTable pitRepository;
LRUContentStore csRepository;
UseCaseInteractor useCaseInteractor(fibRepository, pitRepository, csRepository);
ESP_NOWController espNowController(useCaseInteractor, useCaseInteractor);
uint8_t myMacAddress[6];

// === タイマー関連 ===
constexpr float SENSOR_INTERVAL_SEC = 10.0f;
constexpr float INTEREST_INTERVAL_SEC = 10.0f;
constexpr float AUTO_INTEREST_DELAY_SEC = 40.0f;
constexpr bool AUTO_SENSOR_ENABLED = false; // 起動後の自動センサデータ読み取りを有効にするかどうか
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

/// MACアドレスを "XX:XX:XX:XX:XX:XX" 形式に整形する
/// outLen は最低18（終端文字を含む）必要
void formatMac(const uint8_t *mac, char *out, size_t outLen) {
  if (outLen < 18) {
    if (outLen > 0) out[0] = '\0';
    return;
  }
  snprintf(out, outLen, "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void printMac(const uint8_t *mac) {
  char macStr[18];
  formatMac(mac, macStr, sizeof(macStr));
  LOG_DEBUG(macStr);
}

// === センサデータ送信タスク ===
void readSensorData() {
  LOG_DEBUG("Reading sensor data...");

  const char* contentName = "/iot/buildingA/room101";
  const char* content = "26.5C";
  LOG_INFOF("Sensor: %s = %s\n", contentName, content);
  if (!espNowController.sendSensorData(contentName, content, 1)) {
    LOG_WARN("Failed to process sensor data");
  }
}

// === INTEREST送信 ===
void sendInterest(const uint8_t* targetMac = nullptr) {
  if (targetMac == nullptr) {
    LOG_DEBUG("Sending INTEREST (broadcast)...");
  } else {
    LOG_DEBUG("Sending INTEREST to:");
    printMac(targetMac);
  }

  if (!espNowController.sendInterest("/iot/buildingA/room101", targetMac, 1)) {
    LOG_WARN("Failed to send INTEREST");
  }
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
  LOG_INFO("[AUTO] Starting periodic INTEREST broadcast (10s interval)");
  interestTargetMac = nullptr;
  sendInterest(interestTargetMac);                    // 即座に1回送信
  startInterestTicker();                              // 10秒後から定期送信開始
}

// === ESP-NOW コールバック ===
void onDataSent(const uint8_t * /*mac_addr*/, esp_now_send_status_t status) {
  // This callback keeps warning output compact by design in perf/release-oriented profiles.
  // CLI commands remain available for deeper diagnostics when needed.
  if (status != ESP_NOW_SEND_SUCCESS) {
    LOG_WARNF("[TX] FAIL to peer\n");
  }
}

void onDataReceive(const uint8_t *mac_addr, const uint8_t *data, int len) {
  espNowController.processReceivedPacket(myMacAddress, mac_addr, data, len);
}


void setup() {
  Serial.begin(115200);
  LOG_INFO("Starting setup...");

  const char* configPath = "/config.json";

  if (!espNowController.loadAndApplyConfig(configPath)) {
    LOG_WARN("Failed to load system config!");
    return;
  }

  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    LOG_WARN("ESP-NOW initialization failed");
    return;
  }

  // PMKの設定（暗号化が有効な場合）
  uint8_t pmk[16] = {0};
  if (espNowController.copyPMK(pmk, sizeof(pmk))) {
    if (esp_now_set_pmk(pmk) != ESP_OK) {
      LOG_WARN("Failed to set PMK");
      return;
    }
    LOG_INFO("ESP-NOW encryption enabled (PMK/LMK configured)");
  }

  esp_wifi_get_mac(WIFI_IF_STA, myMacAddress);
  LOG_INFO("My MAC Address:");
  printMac(myMacAddress);

  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataReceive);

  // ブロードキャストアドレスを事前登録
  espNowController.registerBroadcastPeer();

  LOG_INFO("ESP-NOW initialized successfully");

  if (AUTO_SENSOR_ENABLED) {
    sensorTicker.attach(SENSOR_INTERVAL_SEC, onSensorTicker);
    sensorReadRequested = true;  // 起動直後にも1回実行
  } else {
    LOG_DEBUG("[AUTO] Auto sensor read disabled");
  }

  if (AUTO_INTEREST_ENABLED) {
    LOG_INFO("[AUTO] Scheduling INTEREST broadcast to start in 40s");
    autoInterestTicker.once(AUTO_INTEREST_DELAY_SEC, onAutoInterestTicker);
  } else {
    LOG_DEBUG("[AUTO] Auto INTEREST start disabled");
  }

  LOG_INFO("Setup complete.");
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
      LOG_INFO("[CMD] send_interest received - Starting periodic INTEREST broadcast (10s interval)");
      cancelAutoInterestStart();
      interestTargetMac = nullptr;
      sendInterest(interestTargetMac);                    // 即座に1回送信
      startInterestTicker();                              // 10秒後から定期送信開始
    } else if (msg == "stop_interest") {
      LOG_INFO("[CMD] stop_interest received - Stopping periodic INTEREST");
      stopInterestTicker();
      cancelAutoInterestStart();
    } else if (msg == "read_sensor") {
      LOG_INFO("[CMD] read_sensor received");
      readSensorData();
    } else if (msg == "dump_perf") {
      espNowController.dumpPerformanceData();
    } else if (msg == "reset_perf") {
      espNowController.resetPerformanceData();
    } else if (msg == "perf_count") {
      espNowController.printPerformanceCount();
    } else if (msg == "show_counters") {
      LOG_INFO("[CMD] show_counters received");
      espNowController.printCounters();
    } else if (msg == "show_fib") {
      LOG_INFO("[CMD] show_fib received");
      espNowController.printFIB();
    } else if (msg == "clear_cache") {
      LOG_INFO("[CMD] clear_cache received");
      espNowController.clearCSCache();
      espNowController.clearPITCache();
      CLI_PRINTLN("Cache cleared successfully.");
    } else if (msg == "help") {
      CLI_PRINTLN("=== Available Commands ===");
      CLI_PRINTLN("  send_interest   - Start periodic INTEREST broadcast (10s interval)");
      CLI_PRINTLN("  stop_interest   - Stop periodic INTEREST sending");
      CLI_PRINTLN("  read_sensor     - Simulate sensor data send");
      CLI_PRINTLN("  show_counters   - Show tx/rx counter state for all peers");
      CLI_PRINTLN("  show_fib        - Show Forwarding Information Base (FIB)");
      CLI_PRINTLN("  clear_cache     - Clear Content Store and PIT");
      CLI_PRINTLN("  dump_perf       - Dump INTEREST packet timing buffer as JSON (perf build only)");
      CLI_PRINTLN("  reset_perf      - Reset INTEREST packet timing buffer (perf build only)");
      CLI_PRINTLN("  perf_count      - Show current sample count in measurement buffer (perf build only)");
      CLI_PRINTLN("  help            - Show this help");
    } else {
      LOG_WARNF("Unknown command: %s\n", msg.c_str());
      CLI_PRINTLN("Type 'help' to see available commands.");
    }
  }

  // Allow other RTOS tasks and the IDLE task to run, while hardware timers keep firing.
  delay(LOOP_IDLE_DELAY_MS);
}
