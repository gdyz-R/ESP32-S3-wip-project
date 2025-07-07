// components/ai_service/ai_service.h

#pragma once

#include <string>

/**
 * @brief 定义支持的AI模型
 */
enum class AiModel {
    DEEPSEEK,
    COZE
};

/**
 * @brief 初始化AI服务模块
 * 
 * 只需要在程序启动时调用一次。
 */
void ai_service_init();

/**
 * @brief 获取AI模型的回答
 * 
 * 这是一个阻塞函数。它会检查WiFi连接，发送HTTP请求，并等待AI的响应。
 * 
 * @param model 要使用的AI模型 (AiModel::DEEPSEEK 或 AiModel::COZE)。
 * @param input_text 发送给AI的用户问题。
 * @return std::string AI的回答。如果发生错误（如网络问题、API错误、解析失败），
 *         将返回一个以"<错误:"开头的描述性字符串。
 */
std::string get_ai_answer(AiModel model, const std::string& input_text);