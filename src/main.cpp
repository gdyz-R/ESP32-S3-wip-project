#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "audio/my_board.h" // 包含我们定义的板子类
#include <vector>

static const char* TAG = "MAIN";

// 声明板子对象指针
MyBoard* board = nullptr;

// Loopback 任务
void loopback_task(void* pvParameters) {
    ESP_LOGI(TAG, "Loopback task started.");

    // 1. 从板子对象获取 AudioCodec 对象
    // 这完全模仿了 xiaozhi 项目的调用方式！
    AudioCodec* codec = board->GetAudioCodec();
    if (!codec) {
        ESP_LOGE(TAG, "Failed to get audio codec!");
        vTaskDelete(NULL);
        return;
    }

    // 2. 创建一个音频数据缓冲区
    // 大小来自 config.h，这里是 240 个采样点 (int16_t)
    std::vector<int16_t> audio_buffer(AUDIO_CODEC_DMA_FRAME_NUM);

    ESP_LOGI(TAG, "Starting audio loopback... Speak into the microphone!");

    while (1) {
        // 3. 从麦克风读取数据到缓冲区
        if (codec->InputData(audio_buffer)) {
            // 4. 将缓冲区的数据直接写到扬声器
            codec->OutputData(audio_buffer);
        } else {
            // 如果读取失败，稍等一下再试
            vTaskDelay(pdMS_TO_TICKS(5));
        }
    }
}

// 主函数
extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Application starting...");

    // 1. 创建板子对象
    // 在构造函数 MyBoard() 中，所有硬件初始化都会被完成
    board = new MyBoard();

    // 2. 创建并启动 loopback 任务
    xTaskCreate(loopback_task, "loopback_task", 4096, NULL, 5, NULL);
}