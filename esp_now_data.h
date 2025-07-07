#ifndef ESP_NOW_DATA_H
#define ESP_NOW_DATA_H

// 使用 pragma pack 来确保在不同编译器/架构下内存对齐方式一致
#pragma pack(1)

// 定义数据类型，方便主设备区分
typedef enum {
    DATA_TYPE_DHT11,       // 温湿度数据
    DATA_TYPE_GY302,       // 光照强度
    DATA_TYPE_BMP280,      // 气压
    // ... 未来可以添加心率、血氧等
} sensor_type_t;

// 定义温湿度传感器的数据结构
typedef struct {
    float temperature;
    float humidity;
} dht11_data_t;

// 定义光照传感器的数据结构
typedef struct {
    float lux;
} gy302_data_t;

// 定义气压传感器的数据结构
typedef struct {
    float pressure;
} bmp280_data_t;


// 这是最终通过 ESP-NOW 发送的通用消息结构
// 它包含一个类型标识和足够大的联合体来容纳任何一种传感器数据
typedef struct {
    sensor_type_t type;
    union {
        dht11_data_t dht11;
        gy302_data_t gy302;
        bmp280_data_t bmp280;
    } data;
} esp_now_message_t;

#pragma pack()

#endif // ESP_NOW_DATA_H