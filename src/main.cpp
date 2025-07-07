#include <Arduino.h>
#include <M5Unified.h>

void setup() {
    // 1. M5.begin() 应该是第一个调用的函数。
    // 它会初始化所有硬件，包括将 Serial 映射到 USB 虚拟串口。
    auto cfg = M5.config(); // 使用 M5.config() 获取默认配置
    // cfg.internal_imu = true; // 如果需要可以覆盖默认配置
    // cfg.output_power = true;
    M5.begin(cfg);

    // 2. 在 M5.begin() 之后，Serial 对象就已经准备好了。
    // 在 S3 上，等待串口连接是好习惯。
    // 如果监控器已经打开，这个循环会立即跳过。
    while (!Serial) {
        delay(10);
    }

    // 3. 现在可以安全地使用 Serial 进行输出了。
    Serial.println("\n\n--- M5Unified Initialized, Test Begin ---");
}

void loop() {
    // 在 loop 中调用 M5.update() 是一个好习惯，
    // 它可以处理按钮事件等更新。
    M5.update();

    // 使用 printf 可以打印变化的数字，更容易判断程序是否在运行。
    Serial.printf("Looping... Millis: %lu\n", millis());
    delay(1000);
}