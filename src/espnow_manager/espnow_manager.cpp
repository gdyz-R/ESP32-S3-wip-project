#include "espnow_manager.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_log.h"
#include <cstring>

static const char *TAG = "ESPNOW_MANAGER";
static esp_now_data_callback_t s_data_callback = nullptr;

// ESP-NOW 原始接收回调函数 (*** 已更新以匹配新的 API ***)
static void espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
    // 从 recv_info 结构体中获取 MAC 地址
    const uint8_t *mac_addr = recv_info->src_addr;

    if (mac_addr == NULL || data == NULL || len != sizeof(esp_now_message_t)) {
        ESP_LOGE(TAG, "Receive cb error, len=%d", len);
        return;
    }

    esp_now_message_t msg;
    memcpy(&msg, data, sizeof(msg));
    
    if (s_data_callback) {
        s_data_callback(mac_addr, msg);
    }
}


void espnow_manager_init(esp_now_data_callback_t cb) {
    s_data_callback = cb;

    // *** 只保留 ESP-NOW 相关的初始化 ***
    // 确保这个函数在 esp_wifi_init() 和 esp_wifi_set_mode() 之后被调用
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));
    
    ESP_LOGI(TAG, "ESP-NOW callbacks registered.");
}

void espnow_manager_deinit() {
    esp_now_deinit();
    s_data_callback = nullptr;
    ESP_LOGI(TAG, "ESP-NOW Manager De-initialized.");
}