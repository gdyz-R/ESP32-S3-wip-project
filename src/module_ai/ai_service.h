#ifndef AI_SERVICE_H
#define AI_SERVICE_H

#include <string>

// 定义可用的AI模型
enum class AiModel {
    DEEPSEEK,
    COZE
};

/**
 * @brief 初始化AI服务模块。
 */
void ai_service_init();

/**
 * @brief 根据指定的模型和输入文本获取AI的回答。
 * 
 * @param model 要使用的AI模型 (DEEPSEEK 或 COZE)
 * @param input_text 发送给AI的问题或文本
 * @return std::string AI的回答。如果出错，会返回包含错误信息的字符串。
 */
std::string get_ai_answer(AiModel model, const std::string& input_text);

#endif // AI_SERVICE_H