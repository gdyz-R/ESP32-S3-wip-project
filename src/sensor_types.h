#ifndef SENSOR_TYPES_H
#define SENSOR_TYPES_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 环境传感器数据结构
typedef struct {
    float temperature;
    float humidity;
    bool data_valid;
} environment_data_t;

// 为了兼容主程序中的命名
typedef environment_data_t env_sensor_data_t;

// DHT11数据结构（与environment_data_t兼容）
typedef struct {
    float temperature;
    float humidity;
    bool data_valid;
} dht11_data_t;

#ifdef __cplusplus
}
#endif

#endif // SENSOR_TYPES_H