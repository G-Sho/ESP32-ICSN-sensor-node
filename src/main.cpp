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
#include "performance.h"

// === 定数 ===
#define SIGNAL_INTEREST "INTEREST"
#define SIGNAL_DATA "DATA"

// ブロードキャスト定数
constexpr uint8_t BROADCAST_ADDRESS[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// テスト用MACアドレス
constexpr uint8_t TEST_MAC_A[6] = {0xCC, 0x7B, 0x5C, 0x9A, 0xF3, 0xC4};
constexpr uint8_t TEST_MAC_B[6] = {0xCC, 0x7B, 0x5C, 0x9A, 0xF3, 0xAC};
constexpr uint8_t TEST_MAC_C[6] = {0x9C, 0x9C, 0x1F, 0xCF, 0xF4, 0x8C};
constexpr uint8_t BRIDGE_MAC[6] = {0x08, 0xD1, 0xF9, 0x37, 0x39, 0xC0};

// === グローバル ===
ESP_NOWController espNowController;
PeerCounterManager peerCounterManager;
uint8_t myMacAddress[6];

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
  // for (int i = 0; i < 6; i++) {
  //   Serial.printf("%02X", mac[i]);
  //   if (i < 5) Serial.print(":");
  // }
  // Serial.println();
}

/// MACアドレスを改行なしで出力する
void printMacInline(const uint8_t *mac) {
  // for (int i = 0; i < 6; i++) {
  //   Serial.printf("%02X", mac[i]);
  //   if (i < 5) Serial.print(":");
  // }
}

/// パケット内容・カウンタ・HMACを出力する
void printPacket(const CommunicationData &pkt, bool isBcast) {
  // Serial.printf("  signal=%-9s hop=%u  name=%s\n",
  //               pkt.signalCode, pkt.hopCount, pkt.contentName);
  // Serial.printf("  content=%-10s counter=%lu\n",
  //               pkt.content, (unsigned long)pkt.counter);
  // if (!isBcast) {
  //   Serial.print("  hmac=");
  //   for (int i = 0; i < 8; i++) Serial.printf("%02X", pkt.hmac[i]);
  //   Serial.println("... (first 8B)");
  // }
}

/// @brief MAC がピアリスト未登録なら登録する
/// ユニキャスト送受信どちらにも必要。channel=0 で現在の WiFi チャンネルを使用。
void registerPeerIfNeeded(const uint8_t *mac) {
  if (esp_now_is_peer_exist(mac)) return;
  esp_now_peer_info_t p = {};
  memcpy(p.peer_addr, mac, 6);
  p.channel = 0;  // 0 = 現在の WiFi チャンネルを使用（固定指定より確実）
  p.ifidx = WIFI_IF_STA;
  p.encrypt = false;
  if (esp_now_add_peer(&p) != ESP_OK) {
    // Serial.print("[PEER] Failed to register peer: ");
    // printMac(mac);
  }
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

    // ブロードキャスト宛の場合はカウンタ・HMAC処理をスキップ
    bool isBcast = isBroadcastAddress(addr);

    if (!isBcast) {
      // 送信カウンタをインクリメントしてパケットに設定
      bool counterSuccess = false;
      packet.counter = peerCounterManager.incrementTxCounter(addr.data(), counterSuccess);
      if (!counterSuccess) {
        continue;
      }

      // HMAC-SHA256(LMK, パケットデータ（hmacフィールド除く）) を計算してパケットに付与
      memset(packet.hmac, 0, sizeof(packet.hmac));
      bool hmacOk = peerCounterManager.computeHMAC(
          addr.data(),
          reinterpret_cast<const uint8_t*>(&packet),
          COMM_DATA_HMAC_DATA_LEN,
          packet.hmac);
      if (!hmacOk) {
        // Serial.print("[SECURITY] HMAC computation failed for: ");
        // printMac(addr.data());
        continue;
      }
    } else {
      // ブロードキャスト：カウンタはインクリメントしない、HMACも付与しない
      packet.counter = 0;
      memset(packet.hmac, 0, sizeof(packet.hmac));
    }

    // ピアを登録（未登録なら追加）してから送信
    // ※ esp_now_send 直後に del_peer すると WiFi タスクが送信前に peer 情報が消えて FAIL になる
    //   → ピアは削除せず残す（onDataSent でも削除しない）
    registerPeerIfNeeded(addr.data());

    esp_err_t err = esp_now_send(addr.data(), (uint8_t *)&packet, sizeof(packet));
    if (err != ESP_OK) {
      // Serial.printf("[TX] esp_now_send error: %d, to: ", err);
      // printMac(addr.data());
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
  strncpy(sensorData.contentName, "/iot/buildingA/room101", MAX_CONTENT_NAME_LENGTH - 1);
  sensorData.contentName[MAX_CONTENT_NAME_LENGTH - 1] = '\0';
  strncpy(sensorData.content, "26.5C", MAX_CONTENT_LENGTH - 1);
  sensorData.content[MAX_CONTENT_LENGTH - 1] = '\0';

  // Serial.printf("Sensor: %s = %s\n", sensorData.contentName, sensorData.content);
  espNowController.receiveSensorData(sensorData);
}

// === INTEREST送信 ===
void sendInterest(const uint8_t* targetMac = nullptr) {
  // if (targetMac == nullptr) {
  //   Serial.println("Sending INTEREST (broadcast)...");
  // } else {
  //   Serial.print("Sending INTEREST to: ");
  //   printMac(targetMac);
  // }

  ESP_NOWControlData interest = {};
  if (targetMac == nullptr) {
    std::copy(std::begin(BROADCAST_ADDRESS), std::end(BROADCAST_ADDRESS), interest.txAddress[0].begin());
  } else {
    std::copy(targetMac, targetMac + 6, interest.txAddress[0].begin());
  }
  strncpy(interest.signalCode, SIGNAL_INTEREST, MAX_SIGNAL_CODE_LENGTH - 1);
  interest.signalCode[MAX_SIGNAL_CODE_LENGTH - 1] = '\0';
  interest.hopCount = 1;
  strncpy(interest.contentName, "/iot/buildingA/room101", MAX_CONTENT_NAME_LENGTH - 1);
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
  // Serial.println("[AUTO] Starting periodic INTEREST broadcast (10s interval)");
  interestTargetMac = nullptr;
  sendInterest(interestTargetMac);                    // 即座に1回送信
  startInterestTicker();                              // 10秒後から定期送信開始
}

// === ESP-NOW コールバック ===
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status != ESP_NOW_SEND_SUCCESS) {
    // Serial.print("[TX] FAIL to: ");
    // printMac(mac_addr);
  }
}

void onDataReceive(const uint8_t *mac_addr, const uint8_t *data, int len) {
// 送信元をピアリストに登録（未登録だとユニキャストの callback が呼ばれない）
  registerPeerIfNeeded(mac_addr);

  // パケット処理時間測定開始
  MEASURE_START(packet_timer);

  if (len != sizeof(CommunicationData)) {
    MEASURE_END(packet_timer, packetProcessStats);
    return;
  }

  CommunicationData receivedPacket;
  memcpy(&receivedPacket, data, sizeof(CommunicationData));

  // ブロードキャストかユニキャストかを判定
  std::array<uint8_t, 6> macArray;
  std::copy(mac_addr, mac_addr + 6, macArray.begin());
  bool isBroadcast = isBroadcastAddress(macArray);

  // シグナルコードを判定
  bool isInterest = (strncmp(receivedPacket.signalCode, SIGNAL_INTEREST, MAX_SIGNAL_CODE_LENGTH) == 0);
  bool isData    = (strncmp(receivedPacket.signalCode, SIGNAL_DATA,     MAX_SIGNAL_CODE_LENGTH) == 0);

  // 計測ポイント: Interest/Data 受信時刻
  if (isInterest) {
    g_sensor_perf.recordInterestRx();
  } else if (isData) {
    g_sensor_perf.recordDataRx();
  }

  // ユニキャストの場合のみHMAC検証・カウンタ検証を行う
  if (!isBroadcast) {
    // HMAC検証: hmacフィールド以外のパケット全体（counter含む）を対象とする。
    // hmacフィールドはパケット末尾32バイトなので COMM_DATA_HMAC_DATA_LEN の範囲には含まれない。

    // 計測ポイント: OTA（HMAC）検証開始
    if (isInterest) g_sensor_perf.recordOtaStart();

    bool hmacValid = peerCounterManager.verifyHMAC(
        mac_addr,
        reinterpret_cast<const uint8_t*>(&receivedPacket),
        COMM_DATA_HMAC_DATA_LEN,
        receivedPacket.hmac);

    if (!hmacValid) {
      // Serial.print("[SECURITY] HMAC verification FAILED from: ");
      // printMac(mac_addr);
      MEASURE_END(packet_timer, packetProcessStats);
      return;
    }

    if (!peerCounterManager.validateRxCounter(mac_addr, receivedPacket.counter)) {
      // Serial.printf("[SECURITY] Replay attack detected! MAC: ");
      // printMac(mac_addr);
      // Serial.printf("[SECURITY] Expected rx_counter+1, got counter=%lu\n",
      //               (unsigned long)receivedPacket.counter);
      MEASURE_END(packet_timer, packetProcessStats);
      return;
    }

    // 計測ポイント: OTA（HMAC）検証終了
    if (isInterest) g_sensor_perf.recordOtaEnd();
  }

  // ESP_NOWControlData に変換
  ESP_NOWControlData inputData = {};
  strncpy(inputData.signalCode, receivedPacket.signalCode, MAX_SIGNAL_CODE_LENGTH);
  inputData.hopCount = receivedPacket.hopCount;
  strncpy(inputData.contentName, receivedPacket.contentName, MAX_CONTENT_NAME_LENGTH);
  strncpy(inputData.content, receivedPacket.content, MAX_CONTENT_LENGTH);
  std::copy(mac_addr, mac_addr + 6, inputData.txAddress[0].begin());

  ESP_NOWControlData outputData = espNowController.receiveMessage(myMacAddress, mac_addr, inputData);

  // 計測ポイント: FIB検索完了（Interestの場合）
  if (isInterest) g_sensor_perf.recordFibLookup();

  sendPacketToAddresses(outputData);

  // 計測ポイント: 次ホップ送信完了（Interestの場合）
  if (isInterest) g_sensor_perf.recordSensorTx();

  // パケット処理時間測定終了
  MEASURE_END(packet_timer, packetProcessStats);
}


// === パフォーマンスデータJSON出力 ===
void dumpPerformanceData() {
  uint16_t cnt = g_sensor_perf.getCount();
  Serial.println("{");
#if defined(TEST_NODE_ROLE)
  Serial.printf("  \"sensor_role\": %d,\n", TEST_NODE_ROLE);
#else
  Serial.println("  \"sensor_role\": 0,");
#endif
  Serial.println("  \"measurements\": [");
  for (uint16_t i = 0; i < cnt; i++) {
    const SensorMeasurement& m = g_sensor_perf.getEntry(i);
    uint32_t ota_us   = m.ota_end_us - m.ota_start_us;
    uint32_t fib_us   = (m.ota_end_us > 0)
                          ? m.fib_lookup_us - m.ota_end_us
                          : m.fib_lookup_us - m.interest_rx_us;
    uint32_t total_us = m.sensor_tx_us - m.interest_rx_us;
    Serial.printf("    {\"i\": %u, \"ota_us\": %lu, \"fib_us\": %lu, \"total_us\": %lu}",
                  (unsigned)i,
                  (unsigned long)ota_us,
                  (unsigned long)fib_us,
                  (unsigned long)total_us);
    if (i < cnt - 1) Serial.print(",");
    Serial.println();
  }
  Serial.println("  ]");
  Serial.println("}");
}


void setup() {
  Serial.begin(115200);
  // Serial.println("Starting setup...");

  // ノードロールに応じた設定ファイルパスを選択
#if defined(TEST_NODE_ROLE) && TEST_NODE_ROLE == 1
  const char* configPath = "/config_a.json";
  // Serial.println("[ROLE] Sensor A");
#elif defined(TEST_NODE_ROLE) && TEST_NODE_ROLE == 2
  const char* configPath = "/config_b.json";
  // Serial.println("[ROLE] Sensor B");
#elif defined(TEST_NODE_ROLE) && TEST_NODE_ROLE == 3
  const char* configPath = "/config_c.json";
  // Serial.println("[ROLE] Sensor C (data source)");
#else
  const char* configPath = "/config.json";
#endif

  if (!loadSystemConfig(configPath)) {
    // Serial.println("Failed to load system config!");
    return;
  }

  // LMK設定をPeerCounterManagerに反映する
  // グローバルLMK（encryptionEnabled 時のみ有効）
  if (systemConfig.encryptionEnabled) {
    peerCounterManager.setGlobalLMK(systemConfig.lmk);
    // Serial.println("[SECURITY] Global LMK configured for HMAC");
  }
  // ピア固有LMKを設定
  for (size_t i = 0; i < systemConfig.peerLmkCount; i++) {
    const PeerLMKConfig& entry = systemConfig.peerLmkEntries[i];
    if (entry.valid) {
      peerCounterManager.setPeerLMK(entry.mac, entry.lmk);
      // Serial.print("[SECURITY] Peer LMK configured for: ");
      // printMac(entry.mac);
    }
  }

  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    // Serial.println("ESP-NOW initialization failed");
    return;
  }

  // PMKの設定（暗号化が有効な場合）
  if (systemConfig.encryptionEnabled) {
    if (esp_now_set_pmk(systemConfig.pmk) != ESP_OK) {
      // Serial.println("Failed to set PMK");
      return;
    }
    // Serial.println("ESP-NOW encryption enabled (PMK/LMK configured)");
  }

  esp_wifi_get_mac(WIFI_IF_STA, myMacAddress);
  // Serial.print("My MAC Address: ");
  // printMac(myMacAddress);

  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataReceive);

  // 既知ノードを事前登録（自分自身は除く）
  // ユニキャスト受信には送信元がピアリストに登録されている必要があるため
  if (memcmp(myMacAddress, TEST_MAC_A, 6) != 0) registerPeerIfNeeded(TEST_MAC_A);
  if (memcmp(myMacAddress, TEST_MAC_B, 6) != 0) registerPeerIfNeeded(TEST_MAC_B);
  if (memcmp(myMacAddress, TEST_MAC_C, 6) != 0) registerPeerIfNeeded(TEST_MAC_C);
  if (memcmp(myMacAddress, BRIDGE_MAC, 6) != 0) registerPeerIfNeeded(BRIDGE_MAC);
  registerPeerIfNeeded(BROADCAST_ADDRESS);

  // FIB初期エントリの投入（config.jsonの "fib_init" セクションで定義された経路）
  for (size_t i = 0; i < systemConfig.fibInitCount; i++) {
    const FibInitEntry& entry = systemConfig.fibInitEntries[i];
    if (entry.valid) {
      espNowController.initFIBEntry(std::string(entry.contentName),
                                    std::string(entry.nextHopMac));
      // Serial.printf("[FIB] Initial entry: %s -> %s\n",
      //               entry.contentName, entry.nextHopMac);
    }
  }

  // Serial.println("ESP-NOW initialized successfully");

  if (AUTO_SENSOR_ENABLED) {
    sensorTicker.attach(SENSOR_INTERVAL_SEC, onSensorTicker);
    sensorReadRequested = true;  // 起動直後にも1回実行
  } else {
    // Serial.println("[AUTO] Auto sensor read disabled");
  }

  if (AUTO_INTEREST_ENABLED) {
    // Serial.println("[AUTO] Scheduling INTEREST broadcast to start in 40s");
    autoInterestTicker.once(AUTO_INTEREST_DELAY_SEC, onAutoInterestTicker);
  } else {
    // Serial.println("[AUTO] Auto INTEREST start disabled");
  }

  // Serial.println("Setup complete.");
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
      // Serial.println("[CMD] send_interest received - Starting periodic INTEREST broadcast (10s interval)");
      cancelAutoInterestStart();
      interestTargetMac = nullptr;
      sendInterest(interestTargetMac);                    // 即座に1回送信
      startInterestTicker();                              // 10秒後から定期送信開始
    } else if (msg == "send_interest_a") {
      // Serial.println("[CMD] send_interest_a received - Starting periodic INTEREST to MAC A (10s interval)");
      cancelAutoInterestStart();
      interestTargetMac = TEST_MAC_A;
      sendInterest(interestTargetMac);                    // 即座に1回送信
      startInterestTicker();                              // 10秒後から定期送信開始
    } else if (msg == "send_interest_b") {
      // Serial.println("[CMD] send_interest_b received - Starting periodic INTEREST to MAC B (10s interval)");
      cancelAutoInterestStart();
      interestTargetMac = TEST_MAC_B;
      sendInterest(interestTargetMac);                    // 即座に1回送信
      startInterestTicker();                              // 10秒後から定期送信開始
    } else if (msg == "stop_interest") {
      // Serial.println("[CMD] stop_interest received - Stopping periodic INTEREST");
      stopInterestTicker();
      cancelAutoInterestStart();
    } else if (msg == "read_sensor") {
      // Serial.println("[CMD] read_sensor received");
      readSensorData();
    } else if (msg == "perf_stats") {
      // Serial.println("[CMD] perf_stats received");
      packetProcessStats.printStats("Individual Packet Processing");
    } else if (msg == "perf_reset") {
      // Serial.println("[CMD] perf_reset received");
      packetProcessStats.reset();
      // Serial.println("Performance statistics reset.");
    } else if (msg == "dump_perf") {
      dumpPerformanceData();
    } else if (msg == "reset_perf") {
      g_sensor_perf.reset();
      Serial.println("{\"status\": \"perf_reset\"}");
    } else if (msg == "perf_count") {
      Serial.printf("{\"count\": %u}\n", (unsigned)g_sensor_perf.getCount());
    } else if (msg == "show_counters") {
      // Serial.println("[CMD] show_counters received");
      peerCounterManager.printCounters();
    } else if (msg == "show_fib") {
      // Serial.println("[CMD] show_fib received");
      espNowController.printFIB();
    } else if (msg == "clear_cache") {
      // Serial.println("[CMD] clear_cache received");
      espNowController.clearCSCache();
      espNowController.clearPITCache();
      Serial.println("Cache cleared successfully.");
    } else if (msg == "help") {
      // Serial.println("=== Available Commands ===");
      // Serial.println("  send_interest   - Start periodic INTEREST broadcast (10s interval)");
      // Serial.println("  send_interest_a - Start periodic INTEREST to MAC A (10s interval)");
      // Serial.println("  send_interest_b - Start periodic INTEREST to MAC B (10s interval)");
      // Serial.println("  stop_interest   - Stop periodic INTEREST sending");
      // Serial.println("  read_sensor     - Simulate sensor data send");
      // Serial.println("  show_counters   - Show tx/rx counter state for all peers");
      // Serial.println("  show_fib        - Show Forwarding Information Base (FIB)");
      // Serial.println("  clear_cache     - Clear Content Store and PIT");
      // Serial.println("  perf_stats      - Show performance statistics");
      // Serial.println("  perf_reset      - Reset performance statistics");
      // Serial.println("  dump_perf       - Dump sensor measurement buffer as JSON");
      // Serial.println("  reset_perf      - Reset sensor measurement buffer");
      // Serial.println("  perf_count      - Show current sample count in measurement buffer");
      // Serial.println("  help            - Show this help");
    } else {
      // Serial.printf("[WARN] Unknown command: %s\n", msg.c_str());
      // Serial.println("Type 'help' to see available commands.");
    }
  }

  // Allow other RTOS tasks and the IDLE task to run, while hardware timers keep firing.
  delay(LOOP_IDLE_DELAY_MS);
}
