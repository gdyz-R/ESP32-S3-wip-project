// components/ai_service/ai_service.cpp (Arduino Version)

#include "ai_service.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "root_ca.h" // <-- 包含新的根证书头文件

// 日志标签 (在Arduino中，我们通常用Serial打印)
static const char *TAG = "AI_SERVICE";

// ======== 配置信息 (保持不变) ========
static const char* DEEPSEEK_API_KEY = "sk-782f873d727140c791b7c43f6ef88068";
static const char* DEEPSEEK_API_URL = "https://api.deepseek.com/v1/chat/completions";
static const char* COZE_API_KEY = "pat_3hVq6g7PNFQusWWhksJHuLkycKMbBVpje7BCxJ9XCoRkY4Dq8rKLLGJYQFKXKWiH";
static const char* COZE_API_URL = "https://api.coze.cn/open_api/v2/chat";
static const char* COZE_BOT_ID = "7521739543067361306";
static const char* COZE_USER_ID = "esp32-user-123";

// ======== 内部实现函数 ========

/**
 * @brief 内部函数：请求DeepSeek API
 */

// ======== 内部实现函数 (V2 - ArduinoJson v7 兼容版) ========
static std::string _get_deepseek_answer(const std::string& input) {
    std::string result = "<错误: 未知>";
    
    // v7 语法
    JsonDocument doc;
    doc["model"] = "deepseek-chat";
    JsonArray messages = doc["messages"].to<JsonArray>();
    JsonObject system_msg = messages.add<JsonObject>(); // v7 语法
    system_msg["role"] = "system";
    system_msg["content"] = "你是鹏鹏的生活助手，回答控制在256字符内";
    JsonObject user_msg = messages.add<JsonObject>(); // v7 语法
    user_msg["role"] = "user";
    user_msg["content"] = input.c_str();

    std::string json_payload;
    serializeJson(doc, json_payload);

    WiFiClientSecure client;
    client.setCACert(root_ca_pem);
    
    HTTPClient http;
    if (http.begin(client, DEEPSEEK_API_URL)) {
        http.addHeader("Content-Type", "application/json");
        std::string auth_header = "Bearer " + std::string(DEEPSEEK_API_KEY);
        http.addHeader("Authorization", auth_header.c_str());
        http.setTimeout(15000);

        int httpCode = http.POST(json_payload.c_str());

        if (httpCode == HTTP_CODE_OK) {
            String response_body = http.getString();
            
            JsonDocument resp_doc;
            DeserializationError error = deserializeJson(resp_doc, response_body);

            if (!error) {
                // v7 语法: 使用 .is<T>() 检查类型
                if (resp_doc["choices"].is<JsonArray>() && resp_doc["choices"].size() > 0) {
                    const char* content = resp_doc["choices"][0]["message"]["content"];
                    if (content) {
                        result = content;
                    } else {
                        result = "<错误: 找不到content字段>";
                    }
                } else {
                     result = "<错误: 响应中无choices>";
                }
            } else {
                Serial.printf("[%s] JSON解析失败: %s\n", TAG, error.c_str());
                result = "<错误: JSON解析失败>";
            }
        } else {
            Serial.printf("[%s] DeepSeek API 错误, HTTP状态码: %d, 响应: %s\n", TAG, httpCode, http.getString().c_str());
            result = "<错误: API返回非200状态码 " + std::to_string(httpCode) + ">";
        }
        http.end();
    } else {
        Serial.printf("[%s] HTTP 连接失败\n", TAG);
        result = "<错误: HTTP连接失败>";
    }
    
    return result;
}

/**
 * @brief 内部函数：请求Coze API
 */
/**
 * @brief 内部函数：请求Coze API (V2 - ArduinoJson v7 兼容版)
 */
static std::string _get_coze_answer(const std::string& input) {
    std::string result = "<错误: 未知>";
    
    // [ArduinoJson v7 Syntax]
    JsonDocument doc;
    doc["bot_id"] = COZE_BOT_ID;
    doc["user"] = COZE_USER_ID;
    doc["query"] = input.c_str();
    doc["stream"] = false;

    std::string json_payload;
    serializeJson(doc, json_payload);

    // [HTTPClient with Secure WiFi]
    WiFiClientSecure client;
    client.setCACert(root_ca_pem);
    
    HTTPClient http;
    if (http.begin(client, COZE_API_URL)) {
        http.addHeader("Content-Type", "application/json");
        std::string auth_header = "Bearer " + std::string(COZE_API_KEY);
        http.addHeader("Authorization", auth_header.c_str());
        http.setTimeout(30000);

        int httpCode = http.POST(json_payload.c_str());

        if (httpCode == HTTP_CODE_OK) {
            String response_body = http.getString();
            
            // [ArduinoJson v7 Syntax]
            JsonDocument resp_doc;
            DeserializationError error = deserializeJson(resp_doc, response_body);

            if (!error) {
                // v7 语法: 使用 .is<T>() 检查类型
                if (resp_doc["messages"].is<JsonArray>()) {
                    result = ""; // 清空默认错误
                    
                    // v7 语法: 遍历数组
                    for (JsonObject msg : resp_doc["messages"].as<JsonArray>()) {
                        // v7 语法: 检查对象中的键是否存在且为字符串
                        if (msg["content"].is<const char*>()) {
                            result += msg["content"].as<const char*>();
                            result += "\n";
                        }
                    }
                    if (result.empty()) {
                        result = "<错误: 响应中messages为空或无content字段>";
                    } else {
                        // 移除最后一个多余的换行符
                        if (!result.empty() && result.back() == '\n') {
                           result.pop_back();
                        }
                    }
                } else {
                    result = "<错误: 响应中无messages数组>";
                }
            } else {
                Serial.printf("[%s] Coze JSON解析失败: %s\n", TAG, error.c_str());
                result = "<错误: JSON解析失败>";
            }
        } else {
            Serial.printf("[%s] Coze API 错误, HTTP状态码: %d, 响应: %s\n", TAG, httpCode, http.getString().c_str());
            result = "<错误: API返回非200状态码 " + std::to_string(httpCode) + ">";
        }
        http.end();
    } else {
        Serial.printf("[%s] Coze HTTP 连接失败\n", TAG);
        result = "<错误: HTTP连接失败>";
    }

    return result;
}


// ======== 公共接口实现 ========

void ai_service_init() {
    Serial.printf("[%s] AI服务模块已初始化。\n", TAG);
}

std::string get_ai_answer(AiModel model, const std::string& input_text) {
    // [Arduino Conversion]: 使用WiFi.status()检查网络状态
    // 注意: painlessMesh会管理WiFi连接，所以这通常是可靠的
    if (WiFi.status() != WL_CONNECTED) {
        Serial.printf("[%s] 无法发送请求，WiFi未连接！\n", TAG);
        return "<错误: WiFi未连接>";
    }

    if (input_text.empty()) {
        return "<错误: 输入文本不能为空>";
    }

    switch (model) {
        case AiModel::DEEPSEEK:
            Serial.printf("[%s] 向 DeepSeek 发送问题: %s\n", TAG, input_text.c_str());
            return _get_deepseek_answer(input_text);
        case AiModel::COZE:
            Serial.printf("[%s] 向 Coze 发送问题: %s\n", TAG, input_text.c_str());
            return _get_coze_answer(input_text);
        default:
            Serial.printf("[%s] 未知的AI模型类型\n", TAG);
            return "<错误: 未知的AI模型>";
    }
}