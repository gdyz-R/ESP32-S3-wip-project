#include "mqtt_manager.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include <ArduinoJson.h>

// --- 配置 ---
#define MQTT_BROKER_URL "mqtt://47.122.130.135:1883"
#define MQTT_USERNAME   "test"
#define MQTT_PASSWORD   "test123"
#define MQTT_MAIN_DATA_TOPIC "project_data/" // 主题前缀，后面会加上 device_id
#define MQTT_EVENT_TOPIC "project_events/" // 事件主题前缀

// --- 内部变量 ---
static const char *TAG = "MQTT_MANAGER";
static esp_mqtt_client_handle_t client = NULL;
static bool is_mqtt_connected = false;

// --- 事件处理器 (与你提供的版本基本相同，无需修改) ---
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            is_mqtt_connected = true;
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            is_mqtt_connected = false;
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            // 中心板主要是发布，但也可以订阅指令
            ESP_LOGI(TAG, "TOPIC=%.*s", event->topic_len, event->topic);
            ESP_LOGI(TAG, "DATA=%.*s", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
            // 详细的错误日志对于调试非常有帮助
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                ESP_LOGE(TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
            } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
                ESP_LOGE(TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
            } else {
                ESP_LOGW(TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
            }
            break;
        default:
            // 其他事件，如SUBSCRIBED, PUBLISHED等，可以按需添加日志
            break;
    }
}

// --- 公共函数的实现 ---

void mqtt_app_start(void) {
    if (client) {
        ESP_LOGW(TAG, "MQTT client already started.");
        return;
    }

    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = MQTT_BROKER_URL;
    mqtt_cfg.credentials.username = MQTT_USERNAME;
    mqtt_cfg.credentials.authentication.password = MQTT_PASSWORD;
    
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
    
    ESP_LOGI(TAG, "MQTT client started.");
}

void mqtt_app_stop(void) {
    if (client) {
        esp_mqtt_client_stop(client);
        esp_mqtt_client_destroy(client);
        client = NULL;
        is_mqtt_connected = false;
        ESP_LOGI(TAG, "MQTT client stopped and destroyed.");
    }
}

bool mqtt_is_connected(void) {
    return is_mqtt_connected;
}

// 内部发布助手
static void publish_message(const std::string& topic, const std::string& payload, int qos) {
    if (!is_mqtt_connected) {
        ESP_LOGE(TAG, "MQTT not connected. Cannot publish.");
        return;
    }
    esp_mqtt_client_publish(client, topic.c_str(), payload.c_str(), 0, qos, 0);
}


// ============== 这是核心的、重构后的发布函数 ==============
void mqtt_publish_aggregated_data(const std::string& device_id, const AggregatedData& data, uint64_t timestamp) {
    // 推荐使用 StaticJsonDocument，在栈上分配内存，避免堆碎片化，更稳定
    StaticJsonDocument<1024> doc; 

    // 检查每个数据是否有效，如果有效，则添加到JSON中
    if (!std::isnan(data.temp)) {
        JsonObject temp = doc.createNestedObject("temp");
        temp["value"] = data.temp;
        temp["timestamp"] = timestamp;
    }
    if (!std::isnan(data.humi)) {
        JsonObject humi = doc.createNestedObject("humi");
        humi["value"] = data.humi;
        humi["timestamp"] = timestamp;
    }
    if (data.heart != -1) {
        JsonObject heart = doc.createNestedObject("heart");
        heart["value"] = data.heart;
        heart["timestamp"] = timestamp;
    }
    if (data.spo2 != -1) {
        JsonObject spo2 = doc.createNestedObject("spo2");
        spo2["value"] = data.spo2;
        spo2["timestamp"] = timestamp;
    }
    if (data.co2 != -1) {
        JsonObject co2 = doc.createNestedObject("co2");
        co2["value"] = data.co2;
        co2["timestamp"] = timestamp;
    }
    if (data.breathing != -1) {
        JsonObject breathing = doc.createNestedObject("breathing");
        breathing["value"] = data.breathing;
        breathing["timestamp"] = timestamp;
    }

    // 如果没有任何有效数据，则不发送
    if (doc.size() == 0) {
        ESP_LOGW(TAG, "No valid data to publish.");
        return;
    }

    std::string payload_str;
    serializeJson(doc, payload_str);
    
    std::string topic = std::string(MQTT_MAIN_DATA_TOPIC) + device_id;

    ESP_LOGI(TAG, "Publishing to %s: %s", topic.c_str(), payload_str.c_str());
    publish_message(topic, payload_str, 1); // QoS 1 是常规数据的良好选择
}

// ============== 保留的事件发布函数 ==============
void mqtt_publish_event(const std::string& device_id, const std::string& event_type, const std::string& priority) {
    StaticJsonDocument<256> doc;
    doc["event"] = event_type;
    doc["priority"] = priority;

    std::string payload_str;
    serializeJson(doc, payload_str);
    
    std::string topic = std::string(MQTT_EVENT_TOPIC) + device_id;

    ESP_LOGW(TAG, "Publishing EVENT to %s: %s", topic.c_str(), payload_str.c_str());
    publish_message(topic, payload_str, 2); // 事件/警报使用 QoS 2 确保送达
}