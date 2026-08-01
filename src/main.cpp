#include <Adafruit_Sensor.h>
#include <Arduino.h>
#include <Ticker.h>
#include <esp_now.h>

#include "BuildProfile.hpp"
#include "ESP-NOWController.hpp"
#include "MemoryStats.hpp"
#include "Sensor.h"
#include "config/Config.hpp"
#include "infrastructure/data_access/LRUContentStore.hpp"
#include "infrastructure/data_access/LRUForwardingInformationBase.hpp"
#include "infrastructure/data_access/LRUPendingInterestTable.hpp"
#include "infrastructure/data_access/PrefixTreeRIB.hpp"
#include "use_case/UseCaseInteractor.hpp"

// === グローバル ===
LRUForwardingInformationBase fibRepository;
LRUPendingInterestTable pitRepository;
LRUContentStore csRepository;
PrefixTreeRIB ribRepository(fibRepository);
UseCaseInteractor useCaseInteractor(fibRepository, pitRepository, csRepository, ribRepository);
IInputBoundary& inputBoundary = useCaseInteractor;
IForwardingStateBoundary& forwardingStateBoundary = useCaseInteractor;
ESP_NOWController espNowController(inputBoundary, forwardingStateBoundary);
uint8_t myMacAddress[6];

// === タイマー関連 ===
constexpr float SENSOR_INTERVAL_SEC = 10.0f;
constexpr float INTEREST_INTERVAL_SEC = 10.0f;
constexpr float AUTO_INTEREST_DELAY_SEC = 40.0f;
constexpr bool AUTO_SENSOR_ENABLED = false; // 起動後の自動センサデータ読み取りを有効にするかどうか
constexpr bool AUTO_INTEREST_ENABLED = false; // 起動後の自動INTEREST送信を有効にするかどうか
// Allow IDLE task scheduling & reduce active time
constexpr uint32_t LOOP_IDLE_DELAY_MS = 5;
#if ICSN_PERF_ENABLED
constexpr float MEMORY_LOG_INTERVAL_SEC = 30.0f;
#endif

Ticker sensorTicker;
Ticker interestTicker;
Ticker autoInterestTicker;
#if ICSN_PERF_ENABLED
Ticker memoryTicker;
#endif

volatile bool sensorReadRequested = false;
volatile bool interestSendRequested = false;
volatile bool autoInterestStartRequested = false;
#if ICSN_PERF_ENABLED
volatile bool memoryLogRequested = false;
#endif

void IRAM_ATTR onSensorTicker() {
  sensorReadRequested = true;
}
void IRAM_ATTR onInterestTicker() {
  interestSendRequested = true;
}
void IRAM_ATTR onAutoInterestTicker() {
  autoInterestStartRequested = true;
}
#if ICSN_PERF_ENABLED
void IRAM_ATTR onMemoryTicker() {
  memoryLogRequested = true;
}
#endif

void printMemoryUsage(const char* label) {
  const MemorySnapshot snapshot = collectMemorySnapshot();
  printMemorySnapshot(snapshot, label);
  fibRepository.printUsageStats();
  pitRepository.printUsageStats();
  ribRepository.printUsageStats();
  csRepository.printPayloadStats();
}

void printBuildMemoryPolicy() {
  LOG_INFOF("[MEM-POLICY] CS payload psram preferred: %s\n",
            BuildMemoryPolicy::CS_PAYLOAD_PSRAM_PREFERRED ? "true" : "false");
}

void cancelAutoInterestStart() {
  autoInterestTicker.detach();
  autoInterestStartRequested = false;
}

/// MACアドレスを "XX:XX:XX:XX:XX:XX" 形式に整形する
/// outLen は最低18（終端文字を含む）必要
void formatMac(const uint8_t* mac, char* out, size_t outLen) {
  if (outLen < 18) {
    if (outLen > 0)
      out[0] = '\0';
    return;
  }
  snprintf(out, outLen, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4],
           mac[5]);
}

const char* getBuildProfileName() {
#if ICSN_BUILD_PROFILE == ICSN_PROFILE_NORMAL
  return "normal";
#elif ICSN_BUILD_PROFILE == ICSN_PROFILE_PERF
  return "perf";
#elif ICSN_BUILD_PROFILE == ICSN_PROFILE_RELEASE
  return "release";
#else
  return "unknown";
#endif
}

bool isHexDigit(char c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

bool parseMacAddress(const String& text, uint8_t outMac[6]) {
  if (text.length() != 17) {
    return false;
  }

  for (int i = 0; i < 6; i++) {
    const int offset = i * 3;
    if (i > 0 && text.charAt(offset - 1) != ':') {
      return false;
    }

    const char hi = text.charAt(offset);
    const char lo = text.charAt(offset + 1);
    if (!isHexDigit(hi) || !isHexDigit(lo)) {
      return false;
    }

    char bytes[3] = {hi, lo, '\0'};
    char* end = nullptr;
    long value = strtol(bytes, &end, 16);
    if (end == bytes || *end != '\0' || value < 0 || value > 255) {
      return false;
    }

    outMac[i] = static_cast<uint8_t>(value);
  }

  return true;
}

bool isZeroMacAddress(const uint8_t mac[6]) {
  for (size_t i = 0; i < 6; i++) {
    if (mac[i] != 0x00) {
      return false;
    }
  }
  return true;
}

bool isBroadcastOrMulticastMacAddress(const uint8_t mac[6]) {
  bool allFF = true;
  for (size_t i = 0; i < 6; i++) {
    if (mac[i] != 0xFF) {
      allFF = false;
      break;
    }
  }

  return allFF || ((mac[0] & 0x01) != 0);
}

bool isValidContentName(const String& contentName) {
  return contentName.length() > 0 && contentName.charAt(0) == '/';
}

void printTableStatus() {
  CLI_PRINTLN("[TABLES]");
  fibRepository.printUsageStats();
  pitRepository.printUsageStats();
  csRepository.printPayloadStats();
  ribRepository.printUsageStats();
}

void printStatus() {
  char macStr[18];
  formatMac(myMacAddress, macStr, sizeof(macStr));

  CLI_PRINTLN("[DEVICE]");
  CLI_PRINTF("mac=%s\n", macStr);
  CLI_PRINTF("build_profile=%s\n", getBuildProfileName());

  CLI_PRINTLN("\n[SECURITY]");
  CLI_PRINTF("esp_now_ccmp=%s\n", systemConfig.espNowEncryptionEnabled ? "enabled" : "disabled");
  CLI_PRINTF("esp_now_peer_lmk_count=%u\n",
             static_cast<unsigned int>(systemConfig.espNowPeerLmkCount));
  CLI_PRINTF("icsn_hmac=%s\n", systemConfig.hmacAuthenticationEnabled ? "enabled" : "disabled");
  CLI_PRINTF("hmac_peer_key_count=%u\n", static_cast<unsigned int>(systemConfig.hmacPeerKeyCount));

  CLI_PRINTLN("");
  printTableStatus();

  CLI_PRINTLN("\n[MEMORY]");
  const MemorySnapshot snapshot = collectMemorySnapshot();
  printMemorySnapshot(snapshot, "status");
}

void printHelp() {
  CLI_PRINTLN("=== Available Commands ===");
  CLI_PRINTLN("  send_interest <target-mac> [content-name]");
  CLI_PRINTLN("      Send one INTEREST packet to the specified peer");
  CLI_PRINTLN("  stop_interest");
  CLI_PRINTLN("      Stop periodic INTEREST sending");
  CLI_PRINTLN("  read_sensor");
  CLI_PRINTLN("      Simulate sensor data input");
  CLI_PRINTLN("  show_status");
  CLI_PRINTLN("      Show device, security, table and memory status");
  CLI_PRINTLN("  show_fib");
  CLI_PRINTLN("      Show Forwarding Information Base");
  CLI_PRINTLN("  show_pit");
  CLI_PRINTLN("      Show Pending Interest Table");
  CLI_PRINTLN("  show_cs");
  CLI_PRINTLN("      Show Content Store");
  CLI_PRINTLN("  show_counters");
  CLI_PRINTLN("      Show peer TX/RX counters");
  CLI_PRINTLN("  show_mem");
  CLI_PRINTLN("      Show internal RAM and PSRAM status");
  CLI_PRINTLN("  clear_cache");
  CLI_PRINTLN("      Clear Content Store and PIT");
  CLI_PRINTLN("  dump_perf");
  CLI_PRINTLN("      Dump performance data in perf builds");
  CLI_PRINTLN("  reset_perf");
  CLI_PRINTLN("      Reset performance data in perf builds");
  CLI_PRINTLN("  perf_count");
  CLI_PRINTLN("      Show performance sample count");
  CLI_PRINTLN("  help");
  CLI_PRINTLN("      Show this help");
}

void printMac(const uint8_t* mac) {
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
void sendInterest(const uint8_t* targetMac) {
  if (targetMac == nullptr) {
    LOG_WARN("Sending INTEREST requires target MAC (broadcast removed)");
    return;
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

void periodicSendInterest() {
  sendInterest(interestTargetMac);
}

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
  LOG_WARN("[AUTO] Auto INTEREST disabled: target MAC is required (broadcast removed)");
}

// === ESP-NOW コールバック ===
void onDataSent(const uint8_t* mac_addr, esp_now_send_status_t status) {
  // This callback keeps warning output compact by design in perf/release-oriented profiles.
  // CLI commands remain available for deeper diagnostics when needed.
  if (status != ESP_NOW_SEND_SUCCESS) {
    char macStr[18] = "na";
    if (mac_addr != nullptr) {
      formatMac(mac_addr, macStr, sizeof(macStr));
    }
    LOG_WARNF("[WARN][TX] delivery_failed peer=%s\n", macStr);
  }
}

void onDataReceive(const uint8_t* mac_addr, const uint8_t* data, int len) {
  ESP_NOWController::ReceiveProcessResult rxResult;
  if (!espNowController.processReceivedPacket(myMacAddress, mac_addr, data, len, &rxResult)) {
    LOG_DEBUG("[DEBUG][RX] processing_failed");
  }
}

void setup() {
  Serial.begin(115200);
  LOG_INFO("Starting setup...");

  const char* configPath = "/config.json";

  if (!espNowController.initializeCommunication(configPath, myMacAddress, onDataReceive, onDataSent,
                                                1)) {
    LOG_WARN("Failed to initialize communication stack");
    return;
  }

  printBuildMemoryPolicy();

#if ICSN_BUILD_PROFILE != ICSN_PROFILE_RELEASE
  printMemoryUsage("startup");
#endif

  LOG_INFO("My MAC Address:");
  printMac(myMacAddress);

  if (AUTO_SENSOR_ENABLED) {
    sensorTicker.attach(SENSOR_INTERVAL_SEC, onSensorTicker);
    sensorReadRequested = true; // 起動直後にも1回実行
  } else {
    LOG_DEBUG("[AUTO] Auto sensor read disabled");
  }

  if (AUTO_INTEREST_ENABLED) {
    LOG_WARN("[AUTO] AUTO_INTEREST_ENABLED is ignored: target MAC is required");
    autoInterestTicker.once(AUTO_INTEREST_DELAY_SEC, onAutoInterestTicker);
  } else {
    LOG_DEBUG("[AUTO] Auto INTEREST start disabled");
  }

#if ICSN_PERF_ENABLED
  memoryTicker.attach(MEMORY_LOG_INTERVAL_SEC, onMemoryTicker);
#endif

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

#if ICSN_PERF_ENABLED
  if (memoryLogRequested) {
    memoryLogRequested = false;
    printMemoryUsage("periodic(perf)");
  }
#endif

  if (Serial.available() > 0) {
    String msg = Serial.readStringUntil('\n');
    msg.trim();

    if (msg == "send_interest" || msg.startsWith("send_interest ")) {
      String args = msg.substring(String("send_interest").length());
      args.trim();

      if (args.length() == 0) {
        CLI_PRINTLN("[CMD] ERROR: target MAC is required");
        CLI_PRINTLN("Usage: send_interest <target-mac> [content-name]");
      } else {
        int separator = args.indexOf(' ');
        String macText = (separator >= 0) ? args.substring(0, separator) : args;
        String contentName = (separator >= 0) ? args.substring(separator + 1) : String("");
        macText.trim();
        contentName.trim();

        if (contentName.length() == 0) {
          contentName = "/iot/buildingA/room101";
        }

        uint8_t targetMac[6] = {0};
        if (!parseMacAddress(macText, targetMac)) {
          CLI_PRINTLN("[CMD] ERROR: invalid MAC address");
          CLI_PRINTLN("Expected format: XX:XX:XX:XX:XX:XX");
        } else if (isZeroMacAddress(targetMac)) {
          CLI_PRINTLN("[CMD] ERROR: invalid MAC address");
          CLI_PRINTLN("Zero address is not allowed");
        } else if (isBroadcastOrMulticastMacAddress(targetMac)) {
          CLI_PRINTLN("[CMD] ERROR: invalid MAC address");
          CLI_PRINTLN("Broadcast and multicast addresses are not allowed");
        } else if (!isValidContentName(contentName)) {
          CLI_PRINTLN("[CMD] ERROR: invalid Content Name");
          CLI_PRINTLN("Content Name must start with /");
        } else if (!espNowController.sendInterest(contentName.c_str(), targetMac, 1)) {
          CLI_PRINTLN("[CMD] ERROR: INTEREST send failed");
          CLI_PRINTLN("Check peer registration and security configuration.");
        } else {
          char macStr[18];
          formatMac(targetMac, macStr, sizeof(macStr));
          CLI_PRINTLN("[CMD] INTEREST sent");
          CLI_PRINTF("target=%s\n", macStr);
          CLI_PRINTF("content=%s\n", contentName.c_str());
          CLI_PRINTLN("hop_count=1");
        }
      }
      cancelAutoInterestStart();
      stopInterestTicker();
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
    } else if (msg == "show_status") {
      LOG_INFO("[CMD] show_status received");
      printStatus();
    } else if (msg == "show_pit") {
      LOG_INFO("[CMD] show_pit received");
      pitRepository.printCache();
      pitRepository.printUsageStats();
    } else if (msg == "show_cs") {
      LOG_INFO("[CMD] show_cs received");
      csRepository.printCache();
      csRepository.printPayloadStats();
    } else if (msg == "clear_cache") {
      LOG_INFO("[CMD] clear_cache received");
      espNowController.clearCSCache();
      espNowController.clearPITCache();
      CLI_PRINTLN("Cache cleared successfully.");
    } else if (msg == "show_mem") {
      LOG_INFO("[CMD] show_mem received");
      printMemoryUsage("cli");
    } else if (msg == "help") {
      printHelp();
    } else {
      LOG_WARNF("Unknown command: %s\n", msg.c_str());
      CLI_PRINTLN("Type 'help' to see available commands.");
    }
  }

  // Allow other RTOS tasks and the IDLE task to run, while hardware timers keep firing.
  delay(LOOP_IDLE_DELAY_MS);
}
