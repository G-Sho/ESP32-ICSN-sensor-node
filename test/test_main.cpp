#include <Arduino.h>
#include <unity.h>
#include "ArduinoController.h"

// テスト対象のインスタンス
ArduinoController arduinoController;

// ---------- テストケース 1: hop count 超過で discard ----------
void test_hop_count_exceeded()
{
    const char *json = R"({
        "senderId": "node1",
        "signalCode": "INTEREST",
        "contentName": "sensor1/temp",
        "content": "",
        "hopCount": 99,  // 許容外（仮定）
        "time": 123456
    })";

    String result = arduinoController.receiveMessage(1, String(json));
    String signalCode = result["signalCode"];
    TEST_ASSERT_EQUAL_STRING("INVALID", result.c_str()); // 仮に discard を返す仕様とする
}

// ---------- テストケース 2: CS にヒット ----------
void test_content_store_hit()
{
    const char *sensorData = R"({
        "senderId": "nodeX",
        "signalCode": "DATA",
        "contentName": "Goto/birthday",
        "content": "0325",
        "hopCount": 1,
        "time": 1000000000
    })";
    arduinoController.reciveSensorData(sensorData);
    
    const char *json = R"({
        "senderId": "node2",
        "signalCode": "INTEREST",
        "contentName": "Goto/birthday",
        "content": "",
        "hopCount": 1,
        "time": 100000
    })";

    String result = arduinoController.receiveMessage(2, String(json));
    String signalCode = result["signalCode"];
    TEST_ASSERT_EQUAL_STRING("INVALID", result.c_str()); // 仮に discard を返す仕様とする
}

// ---------- テストケース 3: CS miss, FIB hit ----------
void test_forward_via_fib()
{
    arduinoController.addFIBEntry("sensor/humidity", "nextHopNode");

    const char *json = R"({
        "senderId": "nodeB",
        "signalCode": "INTEREST",
        "contentName": "sensor/humidity",
        "content": "",
        "hopCount": 1,
        "time": 200000
    })";

    String result = arduinoController.receiveMessage(123, String(json));
    TEST_ASSERT_EQUAL_STRING("FORWARD:FIB", result.c_str()); // 仮にFORWARD:FIBを返す仕様
}

// ---------- テストケース 4: CS miss, FIB miss ----------
void test_broadcast_interest()
{
    const char *json = R"({
        "senderId": "nodeC",
        "signalCode": "INTEREST",
        "contentName": "sensor/light",
        "content": "",
        "hopCount": 1,
        "time": 300000
    })";

    String result = arduinoController.receiveMessage(123, String(json));
    TEST_ASSERT_EQUAL_STRING("BROADCAST", result.c_str()); // ブロードキャストされたか確認
}

void setup()
{
    delay(1000);
    UNITY_BEGIN();

    RUN_TEST(test_hop_count_exceeded);
    RUN_TEST(test_content_store_hit);
    RUN_TEST(test_forward_via_fib);
    RUN_TEST(test_broadcast_interest);

    UNITY_END();
}

void loop() {}
