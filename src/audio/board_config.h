#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include <driver/gpio.h>
#include "driver/i2s_std.h"

// 和你的 config.h 完全一致
#define AUDIO_INPUT_SAMPLE_RATE  24000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000

#define AUDIO_I2S_GPIO_MCLK GPIO_NUM_NC
#define AUDIO_I2S_GPIO_WS GPIO_NUM_6
#define AUDIO_I2S_GPIO_BCLK GPIO_NUM_8
#define AUDIO_I2S_GPIO_DIN  GPIO_NUM_7
#define AUDIO_I2S_GPIO_DOUT GPIO_NUM_5

#define AUDIO_CODEC_I2C_SDA_PIN  GPIO_NUM_38
#define AUDIO_CODEC_I2C_SCL_PIN  GPIO_NUM_39
// ES8311 芯片的默认 I2C 地址
#define AUDIO_CODEC_ES8311_ADDR  0x18

// Echo Base 上的 I/O 扩展芯片，用于控制功放静音
#define PI4IOE_I2C_ADDR 0x43

// 音频缓冲区大小，来自 audio_codec.h
#define AUDIO_CODEC_DMA_FRAME_NUM 240

#endif // BOARD_CONFIG_H