#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <string>
#include <cstdint> // for uint64_t
#include <cmath>   // for std::isnan

// 聚合数据结构体，用于从子节点接收并发送到MQTT
struct AggregatedData {
    float temp = NAN;
    float humi = NAN;
    int heart = -1;
    int spo2 = -1;
    int co2 = -1;
    int breathing = -1;
};

/**
 * @brief 配置并准备MQTT客户端，但不立即连接。
 */
void mqtt_app_start(void);

/**
 * @brief 断开并清理MQTT客户端。
 */
void mqtt_app_stop(void);

/**
 * @brief 检查MQTT客户端是否已连接到代理。
 * @return true 已连接
 * @return false 未连接
 */
bool mqtt_is_connected(void);

/**
 * @brief MQTT的主循环函数。必须在Arduino的loop()中被频繁调用。
 *        它负责处理消息接收、发送和保持连接。
 */
void mqtt_loop(void);

/**
 * @brief 将聚合的传感器数据发布到MQTT。
 * 
 * @param device_id 设备ID（通常是mesh节点ID）
 * @param data 包含所有传感器读数的AggregatedData结构体
 * @param timestamp 数据的时间戳
 */
void mqtt_publish_aggregated_data(const std::string& device_id, const AggregatedData& data, uint64_t timestamp);

/**
 * @brief 发布一个事件（如警报）到MQTT。
 * 
 * @param device_id 设备ID
 * @param event_type 事件类型（例如 "fall_detected"）
 * @param priority 事件优先级（例如 "high"）
 */
void mqtt_publish_event(const std::string& device_id, const std::string& event_type, const std::string& priority);

#endif // MQTT_MANAGER_H