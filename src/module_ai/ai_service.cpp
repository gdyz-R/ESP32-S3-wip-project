// components/ai_service/ai_service.cpp

#include "ai_service.h"
#include "wifi_manager.h" // 包含您提供的WiFi模块头文件

#include <string>
#include "esp_log.h"
#include "esp_http_client.h"
#include "cJSON.h"

// 日志标签
static const char *TAG = "AI_SERVICE";

// 声明嵌入的根证书变量
extern const unsigned char digicert_global_root_g2_pem_start[] asm("_binary_digicert_global_root_g2_pem_start");
// ======== 配置信息 ========
// DeepSeek 配置
static const char* DEEPSEEK_API_KEY = "sk-782f873d727140c791b7c43f6ef88068";
static const char* DEEPSEEK_API_URL = "https://api.deepseek.com/v1/chat/completions";

// Coze(扣子) 配置
static const char* COZE_API_KEY = "pat_3hVq6g7PNFQusWWhksJHuLkycKMbBVpje7BCxJ9XCoRkY4Dq8rKLLGJYQFKXKWiH";
static const char* COZE_API_URL = "https://api.coze.cn/open_api/v2/chat";
static const char* COZE_BOT_ID = "7521739543067361306";
static const char* COZE_USER_ID = "esp32-user-123"; // 可自定义

// HTTP 事件处理函数，用于接收响应数据
esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
    // 将响应数据追加到 user_data 指向的 string 对象
    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        std::string* response_body = static_cast<std::string*>(evt->user_data);
        response_body->append((char*)evt->data, evt->data_len);
    }
    return ESP_OK;
}

// ======== 内部实现函数 ========

/**
 * @brief 内部函数：请求DeepSeek API
 */
static std::string _get_deepseek_answer(const std::string& input) {
    std::string response_body;
    std::string result = "<错误: 未知>";

    // 1. 构建JSON请求体
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "model", "deepseek-chat");
    cJSON *messages = cJSON_AddArrayToObject(root, "messages");
    cJSON *system_msg = cJSON_CreateObject();
    cJSON_AddStringToObject(system_msg, "role", "system");
    cJSON_AddStringToObject(system_msg, "content", "你是鹏鹏的生活助手，回答控制在256字符内");
    cJSON_AddItemToArray(messages, system_msg);
    cJSON *user_msg = cJSON_CreateObject();
    cJSON_AddStringToObject(user_msg, "role", "user");
    cJSON_AddStringToObject(user_msg, "content", input.c_str());
    cJSON_AddItemToArray(messages, user_msg);
    
    char *json_payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    // 2. 配置HTTP客户端
    std::string auth_header = "Bearer " + std::string(DEEPSEEK_API_KEY);
    esp_http_client_config_t config = {};
    config.url = DEEPSEEK_API_URL;
    config.event_handler = _http_event_handler;
    config.user_data = &response_body;
    config.timeout_ms = 15000;
    // ======== 修复点：使用共享的根CA证书 ========
    config.cert_pem = (const char *)digicert_global_root_g2_pem_start;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Authorization", auth_header.c_str());
    esp_http_client_set_post_field(client, json_payload, strlen(json_payload));

    // 3. 发送请求
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        if (status_code == 200) {
            // 4. 解析JSON响应
            cJSON *json_resp = cJSON_Parse(response_body.c_str());
            if (json_resp) {
                cJSON *choices = cJSON_GetObjectItem(json_resp, "choices");
                if (cJSON_IsArray(choices) && cJSON_GetArraySize(choices) > 0) {
                    cJSON *first_choice = cJSON_GetArrayItem(choices, 0);
                    cJSON *message = cJSON_GetObjectItem(first_choice, "message");
                    cJSON *content = cJSON_GetObjectItem(message, "content");
                    if (cJSON_IsString(content) && (content->valuestring != NULL)) {
                        result = content->valuestring;
                    } else {
                        result = "<错误: 找不到content字段>";
                    }
                } else {
                    result = "<错误: 响应中无choices>";
                }
                cJSON_Delete(json_resp);
            } else {
                result = "<错误: JSON解析失败>";
            }
        } else {
            ESP_LOGE(TAG, "DeepSeek API 错误, HTTP状态码: %d", status_code);
            result = "<错误: API返回非200状态码 " + std::to_string(status_code) + ">";
        }
    } else {
        ESP_LOGE(TAG, "HTTP POST请求失败: %s", esp_err_to_name(err));
        result = "<错误: HTTP请求执行失败>";
    }

    // 5. 清理
    esp_http_client_cleanup(client);
    free(json_payload);
    return result;
}

/**
 * @brief 内部函数：请求Coze API
 */
static std::string _get_coze_answer(const std::string& input) {
    std::string response_body;
    std::string result = "<错误: 未知>";
    
    // 1. 构建JSON请求体
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "bot_id", COZE_BOT_ID);
    cJSON_AddStringToObject(root, "user", COZE_USER_ID);
    cJSON_AddStringToObject(root, "query", input.c_str());
    cJSON_AddBoolToObject(root, "stream", false);

    char* json_payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    // 2. 配置HTTP客户端
    std::string auth_header = "Bearer " + std::string(COZE_API_KEY);
    esp_http_client_config_t config = {};
    config.url = COZE_API_URL;
    config.event_handler = _http_event_handler;
    config.user_data = &response_body;
    config.timeout_ms = 30000;
    // ======== 修复点：使用共享的根CA证书 ========
    config.cert_pem = (const char *)digicert_global_root_g2_pem_start;
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Authorization", auth_header.c_str());
    esp_http_client_set_post_field(client, json_payload, strlen(json_payload));

    // 3. 发送请求
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        if (status_code == 200) {
            // 4. 解析JSON响应
            cJSON *json_resp = cJSON_Parse(response_body.c_str());
            if (json_resp) {
                cJSON *messages = cJSON_GetObjectItem(json_resp, "messages");
                if (cJSON_IsArray(messages)) {
                    result = ""; // 清空默认错误信息
                    cJSON *message_item;
                    cJSON_ArrayForEach(message_item, messages) {
                        cJSON *content = cJSON_GetObjectItem(message_item, "content");
                        if (cJSON_IsString(content) && content->valuestring != NULL) {
                           result += content->valuestring;
                           result += "\n";
                        }
                    }
                    if (result.empty()) result = "<错误: 响应中messages为空>";

                } else {
                    result = "<错误: 响应中无messages数组>";
                }
                cJSON_Delete(json_resp);
            } else {
                result = "<错误: JSON解析失败>";
            }
        } else {
            ESP_LOGE(TAG, "Coze API 错误, HTTP状态码: %d", status_code);
            result = "<错误: API返回非200状态码 " + std::to_string(status_code) + ">";
        }
    } else {
        ESP_LOGE(TAG, "HTTP POST请求失败: %s", esp_err_to_name(err));
        result = "<错误: HTTP请求执行失败>";
    }

    // 5. 清理
    esp_http_client_cleanup(client);
    free(json_payload);
    return result;
}

// ======== 公共接口实现 ========

void ai_service_init() {
    ESP_LOGI(TAG, "AI服务模块已初始化。");
    // 此处可以添加未来可能需要的一次性设置
}

std::string get_ai_answer(AiModel model, const std::string& input_text) {
    // 关键：调用您的WiFi模块检查网络状态
    if (wifi_get_status() != WIFI_STATUS_CONNECTED) {
        ESP_LOGE(TAG, "无法发送请求，WiFi未连接！");
        return "<错误: WiFi未连接>";
    }

    if (input_text.empty()) {
        return "<错误: 输入文本不能为空>";
    }

    switch (model) {
        case AiModel::DEEPSEEK:
            ESP_LOGI(TAG, "向 DeepSeek 发送问题: %s", input_text.c_str());
            return _get_deepseek_answer(input_text);
        case AiModel::COZE:
            ESP_LOGI(TAG, "向 Coze 发送问题: %s", input_text.c_str());
            return _get_coze_answer(input_text);
        default:
            ESP_LOGE(TAG, "未知的AI模型类型");
            return "<错误: 未知的AI模型>";
    }
}