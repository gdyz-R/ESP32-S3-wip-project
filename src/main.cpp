#include <Arduino.h>
#include <ArduinoJson.h>
#include "app_mesh_manager/MeshManager.h" // 包含我们的模块

// 全局 Mesh 管理器实例
MeshManager meshManager;

// 用于存储最终合并的 JSON 字符串
char latest_json[1024] = {0};

/**
 * @brief 数据处理函数
 * 当 MeshManager 收到数据时，会调用这个函数。
 * @param from 发送方节点 ID
 * @param msg 收到的 JSON 字符串
 */
void handleSensorData(uint32_t from, const String &msg) {
    // 这里是你的 JSON 合并逻辑
    // 为了演示，我们只是简单地将最新的消息存储起来
    // 你可以把之前 esp-mesh 版本里的 cJSON 合并逻辑移植到这里
    
    // 示例：解析并打印温度
    StaticJsonDocument<256> doc;
    deserializeJson(doc, msg);
    float temp = doc["payload"]["temp"];
    printf("Parsed temperature from node %u: %.2f C\n", from, temp);

    // 存储最新的完整 JSON
    snprintf(latest_json, sizeof(latest_json), "%s", msg.c_str());
}


extern "C" void app_main(void) {
    // 启用 Arduino 环境
    initArduino();
    
    // 初始化串口用于调试输出
    Serial.begin(115200);

    printf("\n[MAIN] Booting Center Node...\n");

    // 将我们的数据处理函数注册到 MeshManager
    meshManager.onReceive(handleSensorData);

    // 初始化 MeshManager，它会处理所有复杂的网络设置
    meshManager.init();

    printf("[MAIN] Mesh Manager initialized. Entering main loop.\n");

    // 主循环
    while (true) {
        // 必须持续调用 update() 来驱动网络
        meshManager.update();

        // 这里的循环可以做其他事情，比如每隔几秒打印一次状态
        // 为了演示，我们让它空闲，只通过 vTaskDelay 防止任务饿死
        if (latest_json[0] != '\0') {
            printf("[JSON STATUS] %s\n", latest_json);
            // 清空，防止重复打印
            // latest_json[0] = '\0'; 
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}