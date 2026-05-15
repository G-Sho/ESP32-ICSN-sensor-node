#include "Sensor.h"
#include <DHT.h>
#include "BuildProfile.hpp"

constexpr uint8_t dhtPin = 14;     // DHTセンサーが刺さっているピン
constexpr uint8_t dhtType = DHT11; // 今回使うのはDHT11
DHT dht{dhtPin, dhtType};

void DHTHumidity::run()
{
  dht.begin();
}

void DHTHumidity::read()
{
  // 温度を読み取る
  double h = dht.readHumidity();

  // 読み取り失敗
  if (isnan(h))
  {
    LOG_WARNF("Failed to read from DHT sensor\n");
    return;
  }

  m_data = String(h, 3);

  // CSに保存(命名規則については要検討)
  m_numberOfSensorData++;
  if (m_maxSiza <= m_numberOfSensorData)
    m_numberOfSensorData = 0;

  m_contentName = String("/humid/") + String(m_numberOfSensorData);
}

String DHTHumidity::getData()
{
  return m_data;
}

String DHTHumidity::getContentName()
{
  return m_contentName;
}
