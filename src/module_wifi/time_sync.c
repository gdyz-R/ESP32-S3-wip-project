// time_sync.c
#include "esp_sntp.h"
#include "esp_log.h"
#include <time.h>

static const char *TAG = "TIME_SYNC";

void time_sync_notification_cb(struct timeval *tv) {
    ESP_LOGI(TAG, "NTP time synchronized: %s", ctime(&tv->tv_sec));
}

void initialize_sntp(void) {
    ESP_LOGI(TAG, "Initializing SNTP");
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org"); // 使用公共NTP服务器
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    esp_sntp_init();
}

// 在你的 main.c 或 wifi_manager.c 中，当 WiFi 连接成功后调用:
// initialize_sntp();