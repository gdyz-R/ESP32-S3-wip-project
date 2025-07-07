#pragma once // ★ 确认这一行存在

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// 定义所有可能的事件类型
typedef enum {
    // WiFi Events (已存在)
    EVENT_TYPE_WIFI_CONNECTED,
    EVENT_TYPE_WIFI_DISCONNECTED,
    
    // --- 新增：按键事件 ---
    EVENT_TYPE_BUTTON_PRESSED,

    // --- 新增：这是一个演示用的LED控制事件 ---
    EVENT_TYPE_LED_TOGGLE, 

    // ... 为未来模块预留 ...
    EVENT_TYPE_SENSOR_DATA_READY,
    EVENT_TYPE_AI_RESULT_READY,
    EVENT_TYPE_MQTT_COMMAND_RECEIVED,

} app_event_type_t;

// 2. 定义事件消息的结构体
typedef struct {
    app_event_type_t event_type;
    void* event_data; 
} app_event_t;


// 3. 声明一个全局的队列句柄
extern QueueHandle_t g_event_queue;

// 4. 初始化函数声明
void event_queue_init();