// PromptBuilder实现

#include "prompt_builder.h"
#include <sstream>

namespace roboclaw {

PromptBuilder::PromptBuilder()
    : mode_(PromptMode::MINIMAL) {
}

std::string PromptBuilder::getSystemPrompt() const {
    if (!custom_system_prompt_.empty()) {
        return custom_system_prompt_;
    }

    switch (mode_) {
        case PromptMode::MINIMAL:
            return getDefaultSystemPrompt();
        case PromptMode::VERBOSE:
            return getVerboseSystemPrompt();
        case PromptMode::CODING:
            return getDefaultSystemPrompt() + "\n\n你是一个专业的编程助手。在编写代码时，请注意：\n"
                   "- 代码应该清晰、易读、有良好注释\n"
                   "- 遵循最佳实践和设计模式\n"
                   "- 考虑错误处理和边界情况\n";
        case PromptMode::DEBUGGING:
            return getDefaultSystemPrompt() + "\n\n你正在帮助调试代码。请：\n"
                   "- 仔细分析错误信息\n"
                   "- 找出根本原因\n"
                   "- 提供具体的修复建议\n";
        default:
            return getDefaultSystemPrompt();
    }
}

std::string PromptBuilder::getDefaultSystemPrompt() const {
    // Pi风格的极简系统提示词
    return R"(你是RoboClaw，一个AI编程助手。

你可以使用以下工具：
- read(path, offset?, limit?): 读取文件内容
- write(path, content): 创建或覆盖文件
- edit(path, old_string, new_string): 编辑文件（精确替换）
- bash(command, timeout?): 执行shell命令

工具调用格式：{"tool": "read", "path": "文件路径"}
执行工具后，将结果反馈给用户，然后继续你的工作。

重要规则：
1. 修改文件前先用read确认内容
2. edit的old_string必须精确匹配（包括缩进）
3. bash命令超时默认120秒
4. 保持简洁，直接执行任务)";
}

std::string PromptBuilder::getVerboseSystemPrompt() const {
    return R"(你是RoboClaw，一个AI编程助手，基于极简AI Agent框架构建。

## 可用工具

你拥有以下工具来完成用户的任务：

### 1. read - 读取文件
- **参数**：
  - path (string, 必需): 文件路径
  - offset (integer, 可选): 起始行号，默认0
  - limit (integer, 可选): 读取行数，默认全部
- **说明**: 读取文件内容，支持分页读取大文件

### 2. write - 写入文件
- **参数**：
  - path (string, 必需): 文件路径
  - content (string, 必需): 文件内容
- **说明**: 创建新文件或完全覆盖现有文件

### 3. edit - 编辑文件
- **参数**：
  - path (string, 必需): 文件路径
  - old_string (string, 必需): 要替换的内容
  - new_string (string, 必需): 替换后的内容
- **说明**: 精确替换文件中的内容，old_string必须完全匹配

### 4. bash - 执行命令
- **参数**：
  - command (string, 必需): 要执行的命令
  - timeout (integer, 可选): 超时秒数，默认30
- **说明**: 在shell中执行命令，返回stdout和stderr

## 工作流程

1. **理解任务**: 首先理解用户想要完成什么
2. **收集信息**: 使用read工具查看相关文件
3. **执行操作**: 使用write/edit修改文件，或bash执行命令
4. **验证结果**: 确认操作是否成功
5. **反馈用户**: 向用户报告结果

## 重要规则

- ✅ 修改文件前先用read确认当前内容
- ✅ edit的old_string必须精确匹配，包括所有空格和缩进
- ✅ bash命令要谨慎，避免危险操作
- ✅ 每次工具调用后，检查结果再继续
- ❌ 不要盲目执行可能破坏系统的命令
- ❌ 不要假设文件内容，先读取再操作

保持简洁高效，直接完成任务。)";
}

std::vector<ChatMessage> PromptBuilder::buildMessages(
        const std::vector<ChatMessage>& history,
        const std::vector<ToolDefinition>& tools) {

    std::vector<ChatMessage> messages;

    // 添加系统消息
    ChatMessage systemMsg(MessageRole::SYSTEM, getSystemPrompt());

    // 如果有工具，添加工具说明
    if (!tools.empty()) {
        std::string systemContent = systemMsg.content;
        systemContent += "\n\n## 可用工具\n\n";
        for (const auto& tool : tools) {
            systemContent += "### " + tool.name + "\n";
            systemContent += tool.description + "\n\n";
        }
        systemMsg.content = systemContent;
    }

    messages.push_back(systemMsg);

    // 添加历史消息
    for (const auto& msg : history) {
        messages.push_back(msg);
    }

    return messages;
}

std::string PromptBuilder::buildPrompt(
        const std::vector<ChatMessage>& history,
        const std::vector<ToolDefinition>& tools) {

    std::stringstream prompt;

    // 添加系统提示词
    prompt << getSystemPrompt();

    // 添加工具说明
    if (!tools.empty()) {
        prompt << "\n\n## 可用工具\n\n";
        for (const auto& tool : tools) {
            prompt << formatToolDefinition(tool);
        }
    }

    // 添加对话历史
    if (!history.empty()) {
        prompt << "\n\n## 对话历史\n\n";
        prompt << buildHistoryText(history);
    }

    return prompt.str();
}

std::string PromptBuilder::getToolsSchema(const std::vector<ToolDefinition>& tools) const {
    std::stringstream ss;

    ss << "可用工具：\n\n";
    for (const auto& tool : tools) {
        ss << "### " << tool.name << "\n";
        ss << tool.description << "\n";
        ss << "参数：\n";

        if (tool.input_schema.contains("properties")) {
            for (const auto& prop : tool.input_schema["properties"].items()) {
                std::string paramName = prop.key();
                json paramDef = prop.value();

                ss << "  - " << paramName;
                if (paramDef.contains("type")) {
                    ss << " (" << paramDef["type"].get<std::string>() << ")";
                }
                if (paramDef.contains("description")) {
                    ss << ": " << paramDef["description"].get<std::string>();
                }
                ss << "\n";
            }
        }

        ss << "\n";
    }

    return ss.str();
}

std::string PromptBuilder::buildHistoryText(const std::vector<ChatMessage>& history) const {
    std::stringstream ss;

    for (const auto& msg : history) {
        ss << messageToText(msg) << "\n";
    }

    return ss.str();
}

std::string PromptBuilder::formatToolDefinition(const ToolDefinition& tool) const {
    std::stringstream ss;

    ss << "### " << tool.name << "\n";
    ss << tool.description << "\n";

    if (tool.input_schema.contains("properties")) {
        ss << "参数：\n";
        for (const auto& prop : tool.input_schema["properties"].items()) {
            std::string paramName = prop.key();
            json paramDef = prop.value();

            bool required = false;
            if (tool.input_schema.contains("required")) {
                for (const auto& r : tool.input_schema["required"]) {
                    if (r == paramName) {
                        required = true;
                        break;
                    }
                }
            }

            ss << "  - " << paramName;
            if (paramDef.contains("type")) {
                ss << " (" << paramDef["type"].get<std::string>() << ")";
            }
            if (required) {
                ss << " **必需**";
            }
            ss << "\n";
        }
    }

    ss << "\n";

    return ss.str();
}

std::string PromptBuilder::messageToText(const ChatMessage& msg) const {
    std::stringstream ss;

    switch (msg.role) {
        case MessageRole::SYSTEM:
            ss << "[系统]";
            break;
        case MessageRole::USER:
            ss << "[用户]";
            break;
        case MessageRole::ASSISTANT:
            ss << "[助手]";
            break;
        case MessageRole::TOOL:
            ss << "[工具]";
            break;
    }

    ss << " " << msg.content;

    // 显示工具调用
    if (!msg.tool_calls.empty()) {
        ss << "\n[调用工具]: ";
        for (const auto& call : msg.tool_calls) {
            ss << call.name << "(" << call.arguments.dump() << ") ";
        }
    }

    return ss.str();
}

} // namespace roboclaw
