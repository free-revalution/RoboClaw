// Anthropic (Claude) 提供商实现

#ifndef ROBOCLAW_LLM_ANTHROPIC_PROVIDER_H
#define ROBOCLAW_LLM_ANTHROPIC_PROVIDER_H

#include "llm_provider.h"
#include <sstream>

namespace roboclaw {

class AnthropicProvider : public LLMProvider {
public:
    AnthropicProvider(const std::string& apiKey,
                      const std::string& model = "claude-sonnet-4-20250514",
                      const std::string& baseUrl = "");

    ~AnthropicProvider() override = default;

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

    // 转换消息格式（RoboClaw -> Anthropic）
    json convertMessage(const ChatMessage& msg) const;

    // 转换工具格式（RoboClaw -> Anthropic）
    json convertTool(const ToolDefinition& tool) const;

    // 解析响应
    LLMResponse parseResponse(const std::string& body) const;

    // 解析工具调用
    std::vector<ChatMessage::ToolCall> parseToolCalls(const json& content) const;

    // 生成工具调用ID
    std::string generateToolCallId() const;
};

} // namespace roboclaw

#endif // ROBOCLAW_LLM_ANTHROPIC_PROVIDER_H
