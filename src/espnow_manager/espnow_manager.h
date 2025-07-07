#pragma once

#include "../esp_now_data.h" // 引入共享契约文件
#include <functional>
#include <cstdint>
// 定义一个回调函数类型，当收到 ESP-NOW 消息时，会调用这个函数
// 参数1: 来源设备的MAC地址
// 参数2: 收到的消息内容
using esp_now_data_callback_t = std::function<void(const uint8_t* mac_addr, const esp_now_message_t& msg)>;



/**
 * @brief 初始化 ESP-NOW 管理器
 * 
 * @param cb 当收到数据时要调用的回调函数
 */
void espnow_manager_init(esp_now_data_callback_t cb);

/**
 * @brief 反初始化 ESP-NOW 管理器
 */
void espnow_manager_deinit();

// 未来可以添加发送函数，用于双向通信的设备
// void espnow_manager_send(const uint8_t* mac_addr, const esp_now_message_t& msg);