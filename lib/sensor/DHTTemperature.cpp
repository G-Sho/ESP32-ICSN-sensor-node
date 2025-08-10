#include "Sensor.h"
#include <DHT.h>

constexpr uint8_t dhtPin = 14;     // DHTセンサーが刺さっているピン
constexpr uint8_t dhtType = DHT11; // 今回使うのはDHT11
DHT dht{dhtPin, dhtType};

void DHTTemperature::run()
{
  dht.begin();
}

void DHTTemperature::read()
{
  // 温度を読み取る
  double t = dht.readTemperature();

  // 読み取り失敗
  if (isnan(t))
  {
    Serial.println("Failed to read from DHT sensor");
    return;
  }

  m_data = String(t, 3);

  // CSに保存(命名規則については要検討)
  m_numberOfSensorData++;
  if (m_maxSize <= m_numberOfSensorData)
    m_numberOfSensorData = 1;

  m_contentName = String("/temp/") + String(m_numberOfSensorData);
}

String DHTTemperature::getData()
{
  return m_data;
}

String DHTTemperature::getContentName()
{
  return m_contentName;
}