// 提示词构建器 - PromptBuilder
// 构建发送给LLM的提示词

#ifndef ROBOCLAW_AGENT_PROMPT_BUILDER_H
#define ROBOCLAW_AGENT_PROMPT_BUILDER_H

#include "../llm/llm_provider.h"
#include "tool_executor.h"
#include <string>
#include <vector>

namespace roboclaw {

// 提示词模式
enum class PromptMode {
    MINIMAL,      // 极简模式
    VERBOSE,      // 详细模式
    CODING,       // 编程专用
    DEBUGGING     // 调试专用
};

// 提示词构建器
class PromptBuilder {
public:
    PromptBuilder();
    ~PromptBuilder() = default;

    // 设置提示词模式
    void setMode(PromptMode mode) { mode_ = mode; }

    // 构建完整提示词（用于非聊天API）
    std::string buildPrompt(const std::vector<ChatMessage>& history,
                           const std::vector<ToolDefinition>& tools);

    // 构建聊天消息列表（用于聊天API）
    std::vector<ChatMessage> buildMessages(const std::vector<ChatMessage>& history,
                                           const std::vector<ToolDefinition>& tools);

    // 获取系统提示词
    std::string getSystemPrompt() const;

    // 设置自定义系统提示词
    void setSystemPrompt(const std::string& prompt) { custom_system_prompt_ = prompt; }

    // 获取工具Schema
    std::string getToolsSchema(const std::vector<ToolDefinition>& tools) const;

private:
    PromptMode mode_;
    std::string custom_system_prompt_;

    // 获取默认系统提示词
    std::string getDefaultSystemPrompt() const;

    // 获取详细系统提示词
    std::string getVerboseSystemPrompt() const;

    // 构建对话历史文本
    std::string buildHistoryText(const std::vector<ChatMessage>& history) const;

    // 格式化工具定义
    std::string formatToolDefinition(const ToolDefinition& tool) const;

    // 转换消息为文本
    std::string messageToText(const ChatMessage& msg) const;
};

} // namespace roboclaw

#endif // ROBOCLAW_AGENT_PROMPT_BUILDER_H
