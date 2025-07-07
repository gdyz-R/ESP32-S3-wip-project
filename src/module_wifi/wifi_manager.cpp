#include "wifi_manager.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "freertos/event_groups.h"
#include "string.h"
// ****** 请修改为您自己的WiFi账号和密码 ******
#define EXAMPLE_ESP_WIFI_SSID      "Gionix"
#define EXAMPLE_ESP_WIFI_PASS      "u6t4z9ip"
// ******************************************

static const char *TAG = "WIFI_MANAGER";

// 使用 Event Group 来同步连接事件
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
static int s_retry_num = 0;

// 全局变量，用于存储当前WiFi状态
static wifi_status_t g_wifi_status = WIFI_STATUS_DISCONNECTED;

void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        g_wifi_status = WIFI_STATUS_CONNECTING;
        esp_wifi_connect();
        ESP_LOGI(TAG, "Wi-Fi station start, connecting...");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        g_wifi_status = WIFI_STATUS_DISCONNECTED;
        if (s_retry_num < 5) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retry to connect to the AP");
        } else {
            g_wifi_status = WIFI_STATUS_FAILED;
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGI(TAG,"Connect to the AP fail");
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        g_wifi_status = WIFI_STATUS_CONNECTED;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();



    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

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

    wifi_config_t wifi_config = {};
    strcpy((char*)wifi_config.sta.ssid, EXAMPLE_ESP_WIFI_SSID);
    strcpy((char*)wifi_config.sta.password, EXAMPLE_ESP_WIFI_PASS);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );
}

wifi_status_t wifi_get_status(void)
{
    return g_wifi_status;
}

void wifi_print_status(void)
{
    if(g_wifi_status == WIFI_STATUS_CONNECTED) {
        esp_netif_ip_info_t ip_info;
        esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
        esp_netif_get_ip_info(netif, &ip_info);
        ESP_LOGI(TAG, "--- WiFi Status ---");
        ESP_LOGI(TAG, "Status: Connected");
        ESP_LOGI(TAG, "IP: " IPSTR, IP2STR(&ip_info.ip));
        ESP_LOGI(TAG, "-------------------");
    } else {
        ESP_LOGI(TAG, "--- WiFi Status ---");
        ESP_LOGI(TAG, "Status: Disconnected or Failed");
        ESP_LOGI(TAG, "-------------------");
    }
}
// *** 新增的初始化函数，只做一件事：创建事件组 ***
void wifi_manager_init(void) {
    s_wifi_event_group = xEventGroupCreate();
    // 增加一个检查，确保创建成功
    if (s_wifi_event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create event group");
    }
}
// *** 新增一个等待函数 ***
void wifi_wait_for_connected() {
    ESP_LOGI(TAG, "Waiting for WiFi connection...");
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "WiFi Connected to AP");
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Failed to connect to WiFi");
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}