// src/module_wifi/wifi_manager.cpp

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "../config.h"       
#include "../event_queue.h"  
#include "lwip/err.h"
#include "lwip/sys.h"

#include "wifi_manager.h"
#include "../config.h"       // 引用我们的配置文件
#include "../event_queue.h"  // 引用我们的全局事件总线

// 日志标签，方便在输出中识别是哪个模块打印的信息
static const char *TAG = "WIFI_MANAGER";

// 使用一个静态变量来保存当前的WiFi状态
static wifi_status_t s_wifi_status = WIFI_STATUS_DISCONNECTED;

// FreeRTOS 事件组，用于等待IP地址获取
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static int s_retry_num = 0; // 重连次数计数器

// Wi-Fi事件处理函数
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        s_wifi_status = WIFI_STATUS_CONNECTING;
        esp_wifi_connect();
        ESP_LOGI(TAG, "Wi-Fi station started, connecting to AP...");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        s_wifi_status = WIFI_STATUS_DISCONNECTED;
        
        // --- 通过全局事件总线通知其他任务 ---
        app_event_t event = {.event_type = EVENT_TYPE_WIFI_DISCONNECTED, .event_data = NULL};
        xQueueSend(g_event_queue, &event, pdMS_TO_TICKS(10));
        // ------------------------------------

        if (s_retry_num < 10) { // 限制重连次数，例如10次
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retrying to connect to the AP...");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGE(TAG, "Failed to connect to the AP after multiple retries.");
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        s_wifi_status = WIFI_STATUS_CONNECTED;
        s_retry_num = 0; // 连接成功，重置计数器
        ip_event_got_ip_t* event_ip = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP address: " IPSTR, IP2STR(&event_ip->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        
        // --- 通过全局事件总线通知其他任务 ---
        app_event_t event = {.event_type = EVENT_TYPE_WIFI_CONNECTED, .event_data = NULL};
        xQueueSend(g_event_queue, &event, pdMS_TO_TICKS(10));
        // ------------------------------------
    }
}

void wifi_init_sta(void)
{
    // 1. 初始化NVS (非易失性存储)，Wi-Fi驱动需要用它来保存校准数据
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. 创建事件组
    s_wifi_event_group = xEventGroupCreate();

    // 3. 初始化TCP/IP协议栈
    ESP_ERROR_CHECK(esp_netif_init());

    // 4. 创建默认的事件循环
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 5. 创建默认的Wi-Fi Station网络接口
    esp_netif_create_default_wifi_sta();

    // 6. 获取默认的Wi-Fi配置，并初始化
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 7. 注册事件处理函数
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

   
// 8. 配置Wi-Fi参数
wifi_config_t wifi_config; // 1. 先声明一个空的结构体变量

// 2. 使用 memset 将其所有字节清零，这是一个非常好的安全习惯
//    可以防止某些成员没有被初始化而含有随机值。
memset(&wifi_config, 0, sizeof(wifi_config_t));

// 3. 逐个成员进行赋值
//    从config.h中拷贝SSID和密码
strcpy((char *)wifi_config.sta.ssid, WIFI_SSID);
strcpy((char *)wifi_config.sta.password, WIFI_PASSWORD);

//    现在再设置其他参数，这种简单的赋值是C和C++都兼容的
wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
// wifi_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH; // 如果你的ESP-IDF版本支持WPA3，可以保留这行

// 4. 将配置好的结构体应用到Wi-Fi驱动
ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config)); // 注意这里传的是地址

// 关键：禁用WiFi省电模式，提高网络稳定性
esp_wifi_set_ps(WIFI_PS_NONE); 

// 9. 启动Wi-Fi
ESP_ERROR_CHECK(esp_wifi_start());

ESP_LOGI(TAG, "wifi_init_sta finished, waiting for connection...");
EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY);

if (bits & WIFI_CONNECTED_BIT) {
    ESP_LOGI(TAG, "Connected to AP successfully!");
} else if (bits & WIFI_FAIL_BIT) {
    ESP_LOGE(TAG, "Failed to connect to AP!");
} else {
    ESP_LOGE(TAG, "UNEXPECTED EVENT");
}
}


wifi_status_t wifi_get_status(void) {
    return s_wifi_status;
}

void wifi_print_status(void) {
    wifi_status_t current_status = wifi_get_status();
    switch (current_status) {
        case WIFI_STATUS_DISCONNECTED:
            printf("WiFi Status: Disconnected\n");
            break;
        case WIFI_STATUS_CONNECTING:
            printf("WiFi Status: Connecting...\n");
            break;
        case WIFI_STATUS_CONNECTED:
            printf("WiFi Status: Connected\n");
            break;
        case WIFI_STATUS_FAILED:
            printf("WiFi Status: Connection Failed\n");
            break;
        default:
            printf("WiFi Status: Unknown\n");
            break;
    }
}