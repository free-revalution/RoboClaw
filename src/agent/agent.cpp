// Agent实现

#include "agent.h"
#include <sstream>
#include <algorithm>
#include <random>

namespace roboclaw {

Agent::Agent(std::unique_ptr<LLMProvider> llmProvider,
             std::unique_ptr<ToolExecutor> toolExecutor)
    : llm_provider_(std::move(llmProvider))
    , tool_executor_(std::move(toolExecutor)) {
}

AgentResponse Agent::process(const std::string& userMessage) {
    // 创建临时历史（只包含当前消息）
    std::vector<ChatMessage> tempHistory;
    ChatMessage userMsg(MessageRole::USER, userMessage);
    tempHistory.push_back(userMsg);

    return process(userMessage, tempHistory);
}

AgentResponse Agent::process(const std::string& userMessage,
                              const std::vector<ChatMessage>& history) {
    // 添加用户消息到历史
    ChatMessage userMsg(MessageRole::USER, userMessage);
    history_.push_back(userMsg);

    AgentResponse finalResponse;
    int iteration = 0;

    while (iteration < config_.max_iterations) {
        // 构建消息列表
        std::vector<ChatMessage> messages = buildMessages(userMessage);

        // 执行一轮对话
        AgentResponse response = performOneRound(messages);

        // 更新token统计
        finalResponse.total_input_tokens += response.total_input_tokens;
        finalResponse.total_output_tokens += response.total_output_tokens;

        // 添加助手回复到历史
        ChatMessage assistantMsg(MessageRole::ASSISTANT, response.content);
        assistantMsg.tool_calls = response.tool_calls;
        history_.push_back(assistantMsg);

        // 如果没有工具调用，返回结果
        if (!response.has_tool_calls) {
            finalResponse.content = response.content;
            finalResponse.success = response.success;
            finalResponse.error = response.error;
            break;
        }

        // 执行工具调用
        bool allSuccess = executeToolCalls(response.tool_calls);

        // 如果工具执行失败，停止
        if (!allSuccess) {
            finalResponse.error = "工具执行失败";
            finalResponse.success = false;
            break;
        }

        iteration++;
    }

    // 获取最终回复（从最后一条助手消息）
    if (!history_.empty()) {
        for (auto it = history_.rbegin(); it != history_.rend(); ++it) {
            if (it->role == MessageRole::ASSISTANT && !it->content.empty()) {
                finalResponse.content = it->content;
                break;
            }
        }
    }

    finalResponse.success = finalResponse.error.empty();
    return finalResponse;
}

bool Agent::processStream(const std::string& userMessage,
                          std::function<void(const std::string&)> onChunk,
                          std::function<void(const AgentResponse&)> onComplete) {
    // 添加用户消息到历史
    ChatMessage userMsg(MessageRole::USER, userMessage);
    history_.push_back(userMsg);

    // 构建消息列表
    std::vector<ChatMessage> messages = buildMessages(userMessage);

    // 获取工具定义
    std::vector<ToolDescription> toolDescs = tool_executor_->getAllToolDescriptions();
    std::vector<ToolDefinition> tools;
    for (const auto& desc : toolDescs) {
        ToolDefinition tool;
        tool.name = desc.name;
        tool.description = desc.description;
        tool.input_schema = json::parse(desc.parameters.dump());
        tools.push_back(tool);
    }

    // 构建消息
    std::vector<ChatMessage> apiMessages = prompt_builder_.buildMessages(messages, tools);

    // 流式请求
    std::stringstream contentStream;
    bool success = llm_provider_->chatStream(apiMessages, tools,
        [&](const std::string& chunk) {
            // 解析SSE格式的数据
            if (chunk.find("data: ") == 0) {
                std::string data = chunk.substr(6);
                if (data != "[DONE]") {
                    try {
                        json chunkJson = json::parse(data);
                        if (chunkJson.contains("delta")) {
                            auto delta = chunkJson["delta"];
                            if (delta.contains("text")) {
                                std::string text = delta["text"];
                                contentStream << text;
                                onChunk(text);
                            }
                        }
                    } catch (...) {
                        // 忽略解析错误
                    }
                }
            }
        });

    if (!success) {
        AgentResponse response;
        response.success = false;
        response.error = "流式请求失败";
        onComplete(response);
        return false;
    }

    // 构建最终响应
    AgentResponse response;
    response.content = contentStream.str();
    response.success = true;
    response.has_tool_calls = false;

    // 添加助手消息到历史
    ChatMessage assistantMsg(MessageRole::ASSISTANT, response.content);
    history_.push_back(assistantMsg);

    onComplete(response);
    return true;
}

AgentResponse Agent::performOneRound(const std::vector<ChatMessage>& messages) {
    AgentResponse response;

    // 获取工具定义
    std::vector<ToolDescription> toolDescs = tool_executor_->getAllToolDescriptions();
    std::vector<ToolDefinition> tools;
    for (const auto& desc : toolDescs) {
        ToolDefinition tool;
        tool.name = desc.name;
        tool.description = desc.description;
        tool.input_schema = json::parse(desc.parameters.dump());
        tools.push_back(tool);
    }

    // 构建消息
    std::vector<ChatMessage> apiMessages = prompt_builder_.buildMessages(messages, tools);

    // 调用LLM
    LLMResponse llmResponse = llm_provider_->chat(apiMessages, tools);

    if (!llmResponse.success) {
        response.success = false;
        response.error = llmResponse.error;
        return response;
    }

    // 填充响应
    response.content = llmResponse.content;
    response.tool_calls = llmResponse.tool_calls;
    response.has_tool_calls = !llmResponse.tool_calls.empty();
    response.success = true;
    response.total_input_tokens = llmResponse.input_tokens;
    response.total_output_tokens = llmResponse.output_tokens;

    return response;
}

bool Agent::executeToolCalls(const std::vector<ChatMessage::ToolCall>& toolCalls) {
    bool allSuccess = true;

    for (const auto& call : toolCalls) {
        // 执行工具
        ToolResult result = tool_executor_->execute(call.name, call.arguments);

        // 存储结果
        tool_results_[call.id] = result;

        // 添加工具响应消息到历史
        ChatMessage toolMsg(MessageRole::TOOL, result.success ? result.content : result.error);
        toolMsg.tool_call_id = call.id;
        toolMsg.is_error = !result.success;
        history_.push_back(toolMsg);

        if (!result.success) {
            allSuccess = false;
        }
    }

    return allSuccess;
}

std::vector<ChatMessage> Agent::buildMessages(const std::string& userMessage) {
    std::vector<ChatMessage> messages;

    // 添加历史消息（排除最后的用户消息）
    for (size_t i = 0; i < history_.size() - 1; ++i) {
        messages.push_back(history_[i]);
    }

    return messages;
}

bool Agent::shouldContinue(const AgentResponse& response) const {
    // 如果有工具调用，需要继续
    return response.has_tool_calls;
}

} // namespace roboclaw
