// file: src/event_queue.cpp

#include "event_queue.h"

// --- 定义全局事件队列 ---
// 在这里，我们不使用 extern。这是 g_event_queue 变量的实体所在。
QueueHandle_t g_event_queue = NULL;

// --- 实现初始化函数 ---
void event_queue_init(void) {
    // 创建一个队列，可以容纳10个 app_event_t 类型的消息
    g_event_queue = xQueueCreate(10, sizeof(app_event_t));
    if (g_event_queue == NULL) {
        // 实际项目中应该有更健壮的错误处理
        printf("Error: Failed to create event queue!\n");
    }
}