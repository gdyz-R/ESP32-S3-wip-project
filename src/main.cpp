#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include <string>
#include <cmath> // For std::isnan
#include <ctime> 
#include "esp_netif.h"      // <--- 新增
#include "esp_event.h"      // <--- 新增
#include "esp_wifi.h"  
#include "string.h"
// --- 包含我们自己的模块 ---
#include "wifi_manager.h"     // 假设你有一个用于连接WiFi的模块
#include "mqtt_manager.h"     // 你提供的MQTT模块
#include "espnow_manager/espnow_manager.h"   // 我们刚创建的ESPNOW模块
#include "../esp_now_data.h"     // 共享的契约文件
#include "esp_sntp.h"
#include <time.h> // 确保 time.h 被包含
static const char *TAG = "MAIN_APP";
static const std::string DEVICE_ID = "central_hub_01"; // 主设备的ID



static AggregatedData aggregated_data;
// 用于存储所有传感器数据的全局变量
// =========================================================================
// 新增：SNTP 时间同步相关的代码
// =========================================================================
static void time_sync_notification_cb(struct timeval *tv) {
    ESP_LOGI(TAG, "NTP Time synchronized!");
    // (可选) 你可以在这里设置一个标志，表示时间已同步
}

static void initialize_sntp(void) {
    ESP_LOGI(TAG, "Initializing SNTP");
    // 设置 SNTP 工作模式为轮询
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    // 设置 NTP 服务器地址，可以使用国内的服务器以获得更快的响应
    esp_sntp_setservername(0, "ntp.aliyun.com"); // 阿里云NTP服务器
    esp_sntp_setservername(1, "cn.pool.ntp.org");  // 中国NTP池
    esp_sntp_setservername(2, "edu.ntp.org.cn");   // 教育网NTP
    
    // 设置时间同步完成后的回调函数
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    
    // 初始化 SNTP
    esp_sntp_init();

    // 设置本地时区为中国标准时间 (CST-8)
    setenv("TZ", "CST-8", 1);
    tzset();
    ESP_LOGI(TAG, "Timezone set to CST-8");
}


// =========================================================================
// 这是核心逻辑：当 ESP-NOW 收到数据时，这个函数会被调用
// =========================================================================
void handle_espnow_data(const uint8_t* mac_addr, const esp_now_message_t& msg) {
    ESP_LOGI(TAG, "Received data from MAC: %02X:%02X:%02X:%02X:%02X:%02X",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

    // 使用 switch 根据消息类型更新聚合数据
    switch (msg.type) {
        case DATA_TYPE_DHT11:
            ESP_LOGI(TAG, "DHT11 Data - Temp: %.2f, Humi: %.2f", msg.data.dht11.temperature, msg.data.dht11.humidity);
            aggregated_data.temp = msg.data.dht11.temperature;
            aggregated_data.humi = msg.data.dht11.humidity;
            break;
        
        case DATA_TYPE_GY302:
            ESP_LOGI(TAG, "GY302 Data - Lux: %.2f", msg.data.gy302.lux);
            // aggregated_data.lux = msg.data.gy302.lux; // 假设 AggregatedData 中有 lux 字段
            break;

        case DATA_TYPE_BMP280:
             ESP_LOGI(TAG, "BMP280 Data - Pressure: %.2f", msg.data.bmp280.pressure);
             // aggregated_data.pressure = msg.data.bmp280.pressure; // 假设有
            break;

        default:
            ESP_LOGW(TAG, "Received unknown data type: %d", msg.type);
            break;
    }
    
    // 数据更新后，立即调用 MQTT 发布函数
    // 获取当前时间戳 (需要 time.h 和 sntp/time_sync 的支持)
    // 这里为了简单，我们先用一个伪时间戳
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL); // 使用 gettimeofday 获取更精确的时间（秒+微秒）
    uint64_t timestamp = (uint64_t)tv_now.tv_sec * 1000 + (uint64_t)tv_now.tv_usec / 1000;

    if (mqtt_is_connected()) {
        mqtt_publish_aggregated_data(DEVICE_ID, aggregated_data, timestamp);
    } else {
        ESP_LOGE(TAG, "Cannot publish, MQTT is not connected.");
    }
}

extern "C" void app_main(void) {
    // ================== 1. 系统基础初始化 (与之前相同) ==================
    ESP_LOGI(TAG, "===== System Startup =====");
    
    // NVS, netif, event_loop
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    
    // ================== 2. WiFi 和 ESP-NOW 协议栈初始化 ==================
    
    // 步骤 2.1: 初始化 WiFi 协议栈 (但不启动)
    // 这是 esp_wifi_init()，你的 wifi_init_sta() 函数里已经包含了它。
    // 为了不修改 wifi_manager，我们先调用它，但需要理解它内部做了什么。
    // wifi_init_sta() 会做: esp_wifi_init, register_handlers, set_config, start
    // 我们需要把这个流程拆开。
    
    // 为了不修改你的 wifi_manager, 我们采取一个变通但有效的策略：
    // 我们只借用 wifi_manager 的事件处理器，但手动控制初始化顺序。

    ESP_LOGI(TAG, "Initializing WiFi Stack...");
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
 // *** 关键改动：调用 wifi_manager 的初始化函数 ***
    wifi_manager_init(); // 这会创建事件组
    // ================== 3. 注册事件处理器 ==================
    // 从你的 wifi_manager.cpp 中，我们需要 wifi_event_handler 函数。
    // 最好的方式是把它声明在 wifi_manager.h 中。
    // extern "C" void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
    // 假设你已经在 wifi_manager.h 中声明了它
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));
    
    // ================== 4. 设置模式并初始化 ESP-NOW ==================
    ESP_LOGI(TAG, "Setting WiFi mode and Initializing ESP-NOW...");
    // 明确设置 APSTA 模式，兼容性最好
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    
    // 在设置完模式后，立刻初始化 ESP-NOW
    espnow_manager_init(handle_espnow_data);

    // ================== 5. 配置并启动 WiFi 连接 ==================
    ESP_LOGI(TAG, "Configuring and starting WiFi...");
    wifi_config_t wifi_config = {};
    strcpy((char*)wifi_config.sta.ssid, "Gionix"); // 直接在这里写，或者用你的宏
    strcpy((char*)wifi_config.sta.password, "u6t4z9ip");
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    
    // 最后，启动 WiFi
    ESP_ERROR_CHECK(esp_wifi_start());

    // 打印 MAC 地址 (现在肯定能拿到正确的了)
    uint8_t mac[6] = {0};
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    ESP_LOGI(TAG, "Master MAC Address (STA): %02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // ================== 6. 等待连接并启动上层应用 ==================
    // 等待 WiFi 连接成功 (这个函数需要你的 wifi_manager 提供)
    wifi_wait_for_connected();
        initialize_sntp();
    ESP_LOGI(TAG, "Initializing MQTT Client...");
    mqtt_app_start();
    vTaskDelay(2000 / portTICK_PERIOD_MS);

    ESP_LOGI(TAG, "System Initialized. Waiting for ESP-NOW data...");


    // 主任务可以保持运行，或执行其他低优先级任务
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}