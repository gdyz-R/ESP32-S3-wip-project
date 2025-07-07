#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <string>
#include <cmath> // For NAN

// 定义一个结构体来聚合所有传感器的数据
// 使用 NAN (Not A Number) 作为未收到数据的标志，这比用0或-1更安全
struct AggregatedData {
    float temp = NAN;
    float humi = NAN; // 湿度可能也需要
    int heart = -1;   // 对于整数，用-1作为无效标志
    int spo2 = -1;
    int co2 = -1;
    int breathing = -1;
};

// --- MQTT 管理器公共接口 ---

/**
 * @brief 启动并连接到 MQTT broker
 */
void mqtt_app_start(void);

/**
 * @brief 断开并销毁 MQTT 客户端
 */
void mqtt_app_stop(void);

/**
 * @brief 检查 MQTT 是否已连接
 * @return true 如果已连接, false 如果未连接
 */
bool mqtt_is_connected(void);

/**
 * @brief 发布聚合后的传感器数据
 * 
 * @param device_id  设备/床位ID，将用在主题中
 * @param data       包含所有传感器数据的聚合结构体
 * @param timestamp  由主程序提供的、从NTP获取的Unix毫秒时间戳
 */
void mqtt_publish_aggregated_data(const std::string& device_id, const AggregatedData& data, uint64_t timestamp);

/**
 * @brief 发布一个紧急事件/警报 (保留此功能，非常有用)
 * 
 * @param device_id  设备/床位ID
 * @param event_type 事件类型，例如 "fall_detected"
 * @param priority   事件优先级，例如 "high"
 */
void mqtt_publish_event(const std::string& device_id, const std::string& event_type, const std::string& priority);

#endif // MQTT_MANAGER_H