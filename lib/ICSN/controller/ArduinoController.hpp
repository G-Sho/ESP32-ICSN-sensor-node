#pragma once

#include <ArduinoJson.h>
#include "UseCaseInteractor.hpp"
#include "InputData.hpp"
#include "OutputData.hpp"

/// @brief ArduinoController class
/// @details This class is responsible for handling the communication with the Arduino controller.
class ArduinoController
{
private:
  UseCaseInteractor useCaseInteractor;
  painlessMesh *meshPtr = nullptr;

public:
  void setMesh(painlessMesh *meshPtr);
  String receiveMessage(uint32_t to, String msg);
  void reciveSensorData(String msg);

#ifdef UNIT_TEST
  /// @brief CSにデータを追加（テスト用）
  void mockAddToCS(const std::string &name, const std::string &content);

  /// @brief FIBにルート情報を追加（テスト用）
  void mockAddToFIB(const std::string &name, uint32_t nextHop);

  /// @brief PITにエントリを追加（テスト用）
  void mockAddToPIT(const std::string &name, uint32_t fromNode);

  /// @brief センサ読み出しを模倣し、JSONを返す（テスト用）
  String mockReadSensor(const std::string &name, const std::string &content);
#endif
};