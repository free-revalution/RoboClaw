// OpenAI (GPT) 提供商实现

#ifndef ROBOCLAW_LLM_OPENAI_PROVIDER_H
#define ROBOCLAW_LLM_OPENAI_PROVIDER_H

#include "llm_provider.h"

namespace roboclaw {

class OpenAIProvider : public LLMProvider {
public:
    OpenAIProvider(const std::string& apiKey,
                   const std::string& model = "gpt-4o",
                   const std::string& baseUrl = "");

    ~OpenAIProvider() override = default;

    // 发送消息
    LLMResponse chat(const std::vector<ChatMessage>& messages,
                    const std::vector<ToolDefinition>& tools = {}) override;

    // 流式响应
    bool chatStream(const std::vector<ChatMessage>& messages,
                   const std::vector<ToolDefinition>& tools,
                   StreamCallback callback) override;

    // 获取模型名称
    std::string getModelName() const override { return model_; }

    // 设置模型
    void setModel(const std::string& model) override { model_ = model; }

private:
    // 构建请求体
    json buildRequestBody(const std::vector<ChatMessage>& messages,
                         const std::vector<ToolDefinition>& tools) const;

    // 转换消息格式（RoboClaw -> OpenAI）
    json convertMessage(const ChatMessage& msg) const;

    // 转换工具格式（RoboClaw -> OpenAI）
    json convertTool(const ToolDefinition& tool) const;

    // 解析响应
    LLMResponse parseResponse(const std::string& body) const;

    // 生成工具调用ID
    std::string generateToolCallId() const;
};

} // namespace roboclaw

#endif // ROBOCLAW_LLM_OPENAI_PROVIDER_H
