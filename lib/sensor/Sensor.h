#ifndef INCLUDED_Sensor_h_
#define INCLUDED_Sensor_h_
#include <Arduino.h>

class Sensor
{
public:
    virtual void run();
    virtual void read();
    virtual String getData();
    virtual String getContentName();
};

class DHTTemperature : public Sensor
{
public:
    void run() override;
    void read() override;
    String getData() override;
    String getContentName() override;

private:
    String m_data;
    String m_contentName;
    const int m_maxSiza = 100;    
    int m_numberOfSensorData = 0;
};

class DHTHumidity : public Sensor
{
public:
    void run() override;
    void read() override;
    String getData() override;
    String getContentName() override;

private:
    String m_data;
    String m_contentName;
    const int m_maxSiza = 100;    
    int m_numberOfSensorData = 0;
};

#endif // INCLUDED_Sensor_h_