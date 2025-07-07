// components/mqtt_manager/mqtt_manager.cpp (Arduino Version)

#include "mqtt_manager.h"
#include <WiFi.h>
#include <PubSubClient.h> // <-- 新增
#include <ArduinoJson.h>

// --- 配置 (保持不变) ---
#define MQTT_BROKER_IP "47.122.130.135" // PubSubClient 建议用IP
#define MQTT_BROKER_PORT 1883
#define MQTT_USERNAME "test"
#define MQTT_PASSWORD "test123"
#define MQTT_MAIN_DATA_TOPIC "project_data/"
#define MQTT_EVENT_TOPIC "project_events/"
#define MQTT_CLIENT_ID "esp32-main-node" // 最好为每个客户端设置唯一ID

// --- 内部变量 ---
static const char *TAG = "MQTT_MANAGER";
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient); // <-- PubSubClient 实例
long lastReconnectAttempt = 0;

// --- PubSubClient 的消息回调函数 ---
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    Serial.printf("[%s] Message arrived [", TAG);
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();
    // 在这里处理订阅到的消息，如果需要的话
}

// --- 连接和重连逻辑 ---
void reconnect() {
    // 循环直到重新连接
    while (!mqttClient.connected()) {
        Serial.printf("[%s] Attempting MQTT connection...\n", TAG);
        // 尝试连接
        if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
            Serial.printf("[%s] MQTT connected!\n", TAG);
            // 在这里可以重新订阅主题
            // mqttClient.subscribe("some/topic");
        } else {
            Serial.printf("[%s] failed, rc=%d. Try again in 5 seconds\n", TAG, mqttClient.state());
            // 等待5秒再重试
            delay(5000);
        }
    }
}

// --- 公共函数的实现 ---

void mqtt_app_start(void) {
    mqttClient.setServer(MQTT_BROKER_IP, MQTT_BROKER_PORT);
    mqttClient.setCallback(mqtt_callback);
    lastReconnectAttempt = 0;
    Serial.printf("[%s] MQTT client configured.\n", TAG);
}

// 新增一个在主循环中调用的函数
void mqtt_loop() {
    if (!mqttClient.connected()) {
        long now = millis();
        // 每5秒尝试重连一次
        if (now - lastReconnectAttempt > 5000) {
            lastReconnectAttempt = now;
            reconnect();
        }
    }
    mqttClient.loop(); // <-- 非常重要！处理MQTT消息和保持连接
}

void mqtt_app_stop(void) {
    mqttClient.disconnect();
    Serial.printf("[%s] MQTT client disconnected.\n", TAG);
}

bool mqtt_is_connected(void) { return mqttClient.connected(); }

// 内部发布助手
static void publish_message(const std::string& topic, const std::string& payload, bool retained) {
    if (!mqtt_is_connected()) {
        Serial.printf("[%s] MQTT not connected. Cannot publish.\n", TAG);
        return;
    }
    mqttClient.publish(topic.c_str(), payload.c_str(), retained);
}

// ============== 这是核心的、重构后的发布函数 (V3 - ArduinoJson v7 兼容版) ==============
void mqtt_publish_aggregated_data(const std::string& device_id, const AggregatedData& data, uint64_t timestamp) {
    // ArduinoJson v7: 直接使用 JsonDocument
    JsonDocument doc; 

    if (!std::isnan(data.temp)) {
        // v7 语法: doc["key"].to<JsonObject>()
        JsonObject temp_obj = doc["temp"].to<JsonObject>(); 
        // v7 语法: 直接赋值，ArduinoJson会自动处理浮点数精度
        temp_obj["value"] = data.temp;
        temp_obj["timestamp"] = timestamp;
    }
    if (!std::isnan(data.humi)) {
        JsonObject humi_obj = doc["humi"].to<JsonObject>();
        humi_obj["value"] = data.humi;
        humi_obj["timestamp"] = timestamp;
    }
    if (data.heart != -1) {
        JsonObject heart_obj = doc["heart"].to<JsonObject>();
        heart_obj["value"] = data.heart;
        heart_obj["timestamp"] = timestamp;
    }
    if (data.spo2 != -1) {
        JsonObject spo2_obj = doc["spo2"].to<JsonObject>();
        spo2_obj["value"] = data.spo2;
        spo2_obj["timestamp"] = timestamp;
    }
    if (data.co2 != -1) {
        JsonObject co2_obj = doc["co2"].to<JsonObject>();
        co2_obj["value"] = data.co2;
        co2_obj["timestamp"] = timestamp;
    }
    if (data.breathing != -1) {
        JsonObject breathing_obj = doc["breathing"].to<JsonObject>();
        breathing_obj["value"] = data.breathing;
        breathing_obj["timestamp"] = timestamp;
    }

    if (doc.isNull()) {
        Serial.printf("[%s] No valid data to publish for device %s.\n", TAG, device_id.c_str());
        return;
    }

    std::string payload_str;
    serializeJson(doc, payload_str);
    
    std::string topic = std::string(MQTT_MAIN_DATA_TOPIC) + device_id;

    Serial.printf("[%s] Publishing to %s: %s\n", TAG, topic.c_str(), payload_str.c_str());
    publish_message(topic, payload_str, false);
}

// 事件发布函数 (V3 - ArduinoJson v7 兼容版)
void mqtt_publish_event(const std::string& device_id, const std::string& event_type, const std::string& priority) {
    JsonDocument doc;
    doc["event"] = event_type;
    doc["priority"] = priority;

    std::string payload_str;
    serializeJson(doc, payload_str);
    
    std::string topic = std::string(MQTT_EVENT_TOPIC) + device_id;

    Serial.printf("[%s] Publishing EVENT to %s: %s\n", TAG, topic.c_str(), payload_str.c_str());
    publish_message(topic, payload_str, false);
}