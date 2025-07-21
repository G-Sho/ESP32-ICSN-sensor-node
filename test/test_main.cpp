#include <Arduino.h>
#include <unity.h>
#include <ArduinoJson.h>
#include "controller/ArduinoController.hpp"

ArduinoController controller;

// ---------- Interest受信テスト ----------

// TC1: CS Hit → Data応答
void test_interest_cs_hit()
{
    controller.mockAddToCS("/sensor/temp", "23C");

    String interest = R"({
        "signalCode": "INTEREST",
        "contentName": "/sensor/temp",
        "hopCount": 3,
        "destId":["-1"]
    })";

    String result = controller.receiveMessage(0, interest);
    TEST_ASSERT_NOT_EQUAL(-1, result.indexOf("DATA"));
    TEST_ASSERT_NOT_EQUAL(-1, result.indexOf("/sensor/temp"));
}

// TC2: CS Miss + FIB Hit → Interest転送
void test_interest_fib_hit()
{
    controller.mockAddToFIB("/sensor/temp", 123);

    String interest = R"({
        "signalCode": "INTEREST",
        "contentName": "/sensor/temp",
        "hopCount": 2,
        "destId":["-1"]
    })";

    String result = controller.receiveMessage(0, interest);
    TEST_ASSERT_NOT_EQUAL(-1, result.indexOf("INTEREST"));
    TEST_ASSERT_NOT_EQUAL(-1, result.indexOf("/sensor/temp"));
}

// TC3: CS Miss + FIB Miss → 廃棄
void test_interest_drop_no_fib_cs()
{
    String interest = R"({
        "signalCode": "INTEREST",
        "contentName": "/unknown/path",
        "hopCount": 3,
        "destId":["-1"]
    })";

    String result = controller.receiveMessage(0, interest);
    TEST_ASSERT_TRUE(result.length() == 0 || result.indexOf("INVALID") != -1);
}

// TC4: hopCount 0 → 廃棄
void test_interest_hop_zero()
{
    String interest = R"({
        "signalCode": "INTEREST",
        "contentName": "/sensor/light",
        "hopCount": 0,
        "destId":["-1"]
    })";

    String result = controller.receiveMessage(0, interest);
    TEST_ASSERT_TRUE(result.length() == 0 || result.indexOf("INVALID") != -1);
}

// ---------- Data受信テスト ----------

// TC5: PIT Hit → Data転送 + CS保存
void test_data_with_pit_hit()
{
    controller.mockAddToPIT("/sensor/humidity", 789);

    String data = R"({
        "signalCode": "DATA",
        "contentName": "/sensor/humidity",
        "content": "40%",
        "time": 12345,
        "destId": ["-1"]
    })";

    String result = controller.receiveMessage(0, data);
    TEST_ASSERT_NOT_EQUAL(-1, result.indexOf("DATA"));
    TEST_ASSERT_NOT_EQUAL(-1, result.indexOf("humidity"));
}

// TC6: PIT Miss → 廃棄（INVALID応答または空）
void test_data_with_pit_miss()
{
    String data = R"({
        "signalCode": "DATA",
        "contentName": "/sensor/unknown",
        "content": "NA",
        "time": 1000,
        "destId": ["-1"]
    })";

    String result = controller.receiveMessage(0, data);
    TEST_ASSERT_TRUE(result.length() == 0 || result.indexOf("INVALID") != -1);
}

// ---------- Sensor読み出し ----------

// TC7: 正常なSensor読み取り
void test_sensor_data_reading()
{
    String sensorData = controller.mockReadSensor("/sensor/light", "100lx");

    JsonDocument testDoc;
    DeserializationError err = deserializeJson(testDoc, sensorData);
    TEST_ASSERT_FALSE(err);
    TEST_ASSERT_EQUAL_STRING("/sensor/light", testDoc["contentName"]);
    TEST_ASSERT_EQUAL_STRING("100lx", testDoc["content"]);
    TEST_ASSERT_TRUE(testDoc["time"].is<int>() || testDoc["time"].is<long>() || testDoc["time"].is<unsigned long>());
}

// TC8: センサが空文字を返す → 無効処理
void test_sensor_data_invalid()
{
    String sensorData = controller.mockReadSensor("/sensor/empty", "");

    JsonDocument testDoc;
    DeserializationError err = deserializeJson(testDoc, sensorData);
    TEST_ASSERT_FALSE(err);
    TEST_ASSERT_EQUAL_STRING("", testDoc["content"]);
}

// ---------- Unity用 setup / loop ----------

void setup()
{
    UNITY_BEGIN();

    RUN_TEST(test_interest_cs_hit);
    RUN_TEST(test_interest_fib_hit);
    RUN_TEST(test_interest_drop_no_fib_cs);
    RUN_TEST(test_interest_hop_zero);

    RUN_TEST(test_data_with_pit_hit);
    RUN_TEST(test_data_with_pit_miss);

    RUN_TEST(test_sensor_data_reading);
    RUN_TEST(test_sensor_data_invalid);

    UNITY_END();
}

void loop()
{
    // Not used in tests
}
