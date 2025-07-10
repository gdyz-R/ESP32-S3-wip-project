#ifndef MY_BOARD_H
#define MY_BOARD_H

#include <vector>
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "driver/i2s_std.h"
#include "board_config.h"
#include "freertos/FreeRTOS.h" // 引入 FreeRTOS 头文件
#include "freertos/semphr.h"   // 引入信号量/互斥锁头文件

class AudioCodec {
public:
    virtual ~AudioCodec() {}
    virtual void Init() = 0;
    virtual bool InputData(std::vector<int16_t>& data) = 0;
    virtual void OutputData(const std::vector<int16_t>& data) = 0;
};

class MyEs8311Codec : public AudioCodec {
private:
    i2c_master_bus_handle_t i2c_bus_handle_;
    i2s_chan_handle_t rx_handle_ = NULL; // 初始化为 NULL
    i2s_chan_handle_t tx_handle_ = NULL; // 初始化为 NULL
    const char* TAG = "MyEs8311Codec";

public:
    MyEs8311Codec(i2c_master_bus_handle_t bus_handle) : i2c_bus_handle_(bus_handle) {}

    void Init() override {
        ESP_LOGI(TAG, "Initializing Full-Duplex I2S Driver...");
        
        // 1. 配置 I2S 通道参数
        i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
        
        // 2. **关键修正**: 创建全双工通道
        // 将 tx_handle 和 rx_handle 同时传入，驱动就会创建一对全双工通道
        ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_handle_, &rx_handle_));

        // 3. 配置 I2S 标准模式
        // Echo Base 的麦克风和扬声器使用相同的时钟，所以只需配置一次
        i2s_std_config_t std_cfg = {
            .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(AUDIO_OUTPUT_SAMPLE_RATE),
            .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
            .gpio_cfg = {
                .mclk = AUDIO_I2S_GPIO_MCLK,
                .bclk = AUDIO_I2S_GPIO_BCLK,
                .ws = AUDIO_I2S_GPIO_WS,
                .dout = AUDIO_I2S_GPIO_DOUT,
                .din = AUDIO_I2S_GPIO_DIN,
                .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false },
            },
        };
        
        // 4. 初始化这对全双工通道
        ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_handle_, &std_cfg));
        ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle_, &std_cfg));

        // 5. 启动 I2S
        ESP_ERROR_CHECK(i2s_channel_enable(tx_handle_));
        ESP_ERROR_CHECK(i2s_channel_enable(rx_handle_));
        
        ESP_LOGI(TAG, "I2S Driver Started in Full-Duplex mode.");
        ESP_LOGI(TAG, "ES8311 Codec configured via I2C (simulated).");
    }

    // InputData 和 OutputData 函数无需修改
    bool InputData(std::vector<int16_t>& data) override {
        size_t bytes_read = 0;
        esp_err_t ret = i2s_channel_read(rx_handle_, data.data(), data.size() * sizeof(int16_t), &bytes_read, pdMS_TO_TICKS(100));
        if (ret != ESP_OK && ret != ESP_ERR_TIMEOUT) {
            ESP_LOGE(TAG, "I2S Read Error: %s", esp_err_to_name(ret));
            return false;
        }
        return bytes_read > 0;
    }

    void OutputData(const std::vector<int16_t>& data) override {
        size_t bytes_written = 0;
        esp_err_t ret = i2s_channel_write(tx_handle_, data.data(), data.size() * sizeof(int16_t), &bytes_written, portMAX_DELAY);
         if (ret != ESP_OK) {
            ESP_LOGE(TAG, "I2S Write Error: %s", esp_err_to_name(ret));
        }
    }
};

// MyBoard 类无需修改，保持原样
class MyBoard {
private:
    i2c_master_bus_handle_t i2c_bus_handle_;
    AudioCodec* audio_codec_ = nullptr;
    const char* TAG = "MyBoard";

    void InitializeI2c() {
        ESP_LOGI(TAG, "Initializing I2C Bus...");
        i2c_master_bus_config_t i2c_bus_cfg = {
            .i2c_port = I2C_NUM_1,
            .sda_io_num = AUDIO_CODEC_I2C_SDA_PIN,
            .scl_io_num = AUDIO_CODEC_I2C_SCL_PIN,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .flags = { .enable_internal_pullup = true },
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &i2c_bus_handle_));
    }

    void InitializePi4ioe() {
        ESP_LOGI(TAG, "Initializing PI4IOE and unmuting speaker...");
        i2c_master_dev_handle_t pi4ioe_dev_handle;
        i2c_device_config_t dev_cfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = PI4IOE_I2C_ADDR,
            .scl_speed_hz = 100000,
        };
        esp_err_t err = i2c_master_bus_add_device(i2c_bus_handle_, &dev_cfg, &pi4ioe_dev_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to add PI4IOE device: %s", esp_err_to_name(err));
            return;
        }
        uint8_t unmute_cmd[] = {0x05, 0xFF};
        err = i2c_master_transmit(pi4ioe_dev_handle, unmute_cmd, sizeof(unmute_cmd), pdMS_TO_TICKS(100));
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to unmute speaker via PI4IOE: %s", esp_err_to_name(err));
        } else {
            ESP_LOGI(TAG, "Speaker unmuted successfully.");
        }
        ESP_ERROR_CHECK(i2c_master_bus_rm_device(pi4ioe_dev_handle));
    }

public:
    MyBoard() {
        InitializeI2c();
        InitializePi4ioe();
        audio_codec_ = new MyEs8311Codec(i2c_bus_handle_);
        audio_codec_->Init();
    }

    AudioCodec* GetAudioCodec() {
        return audio_codec_;
    }
};

#endif // MY_BOARD_H