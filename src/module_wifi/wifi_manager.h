// src/module_wifi/wifi_manager.h

#pragma once

#include "esp_wifi.h"

// 定义一个枚举来表示WiFi状态，方便外部调用
typedef enum {
    WIFI_STATUS_DISCONNECTED,
    WIFI_STATUS_CONNECTING,
    WIFI_STATUS_CONNECTED,
    WIFI_STATUS_FAILED
} wifi_status_t;

/**
 * @brief 初始化Wi-Fi管理器
 * 
 * 该函数会初始化底层的TCP/IP协议栈和Wi-Fi驱动，并启动连接过程。
 * 只需要在 app_main 中调用一次。
 */
void wifi_init_sta(void);

/**
 * @brief 获取当前的Wi-Fi连接状态
 * 
 * 这是一个测试和状态查询函数，可以在任何地方调用以获取当前状态。
 * @return wifi_status_t 当前的WiFi状态
 */
wifi_status_t wifi_get_status(void);

/**
 * @brief (测试函数) 打印当前的Wi-Fi状态到串口
 * 
 * 可以在main.cpp中调用此函数进行快速测试。
 */
void wifi_print_status(void);