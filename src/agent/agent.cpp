// Agent实现

#include "agent.h"
#include "../utils/logger.h"
#include <sstream>
#include <algorithm>
#include <random>
#include <shared_mutex>
#include <mutex>
#include <future>
#include <utility>
#include "task_coordinator.h"

namespace roboclaw {

Agent::Agent(std::unique_ptr<LLMProvider> llmProvider,
             std::unique_ptr<ToolExecutor> toolExecutor)
    : llm_provider_(std::move(llmProvider))
    , tool_executor_(std::move(toolExecutor))
    , task_coordinator_(std::make_shared<agent::TaskCoordinator>())
    , token_optimization_enabled_(false) {
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
    {
        std::unique_lock<std::shared_mutex> lock(history_mutex_);
        history_.push_back(userMsg);
    }

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
        {
            std::unique_lock<std::shared_mutex> lock(history_mutex_);
            history_.push_back(assistantMsg);
        }

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
    {
        std::shared_lock<std::shared_mutex> lock(history_mutex_);
        if (!history_.empty()) {
            for (auto it = history_.rbegin(); it != history_.rend(); ++it) {
                if (it->role == MessageRole::ASSISTANT && !it->content.empty()) {
                    finalResponse.content = it->content;
                    break;
                }
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
    {
        std::unique_lock<std::shared_mutex> lock(history_mutex_);
        history_.push_back(userMsg);
    }

    // 构建消息列表
    std::vector<ChatMessage> messages = buildMessages(userMessage);

    // 获取工具定义
    std::vector<ToolDescription> toolDescs = tool_executor_->getAllToolDescriptions();
    std::vector<ToolDefinition> tools;
    for (const auto& desc : toolDescs) {
        ToolDefinition tool;
        tool.name = desc.name;
        tool.description = desc.description;

        // 构建 JSON Schema 格式的 parameters
        json paramsSchema;
        paramsSchema["type"] = "object";

        // 构建 properties
        json properties = json::object();
        std::vector<std::string> required;
        for (const auto& param : desc.parameters) {
            json paramSchema;
            paramSchema["type"] = param.type;
            paramSchema["description"] = param.description;

            // 添加默认值（如果有）
            if (!param.default_value.empty()) {
                if (param.type == "string") {
                    paramSchema["default"] = param.default_value;
                } else if (param.type == "integer") {
                    try {
                        paramSchema["default"] = std::stoi(param.default_value);
                    } catch (...) {
                        // 保持默认为字符串
                    }
                }
            }

            properties[param.name] = paramSchema;

            // 记录必需参数
            if (param.required) {
                required.push_back(param.name);
            }
        }
        paramsSchema["properties"] = properties;

        // 添加 required 数组（如果有必需参数）
        if (!required.empty()) {
            paramsSchema["required"] = required;
        }

        tool.input_schema = paramsSchema;
        tools.push_back(tool);
    }

    // 构建消息
    std::vector<ChatMessage> apiMessages = prompt_builder_.buildMessages(messages, tools);

    // 流式请求（注意：当前postStream实现实际返回完整JSON，不是真正的SSE流）
    std::stringstream contentStream;
    std::vector<ChatMessage::ToolCall> accumulatedToolCalls;
    bool success = llm_provider_->chatStream(apiMessages, tools,
        [&](const std::string& chunk) {
            // 当前实现返回完整JSON响应，不是SSE格式
            try {
                json responseJson = json::parse(chunk);

                // 检查是否有choices
                if (responseJson.contains("choices") && !responseJson["choices"].empty()) {
                    auto choice = responseJson["choices"][0];

                    // 获取文本内容
                    if (choice.contains("message")) {
                        auto message = choice["message"];

                        // 处理普通文本内容
                        if (message.contains("content") && !message["content"].is_null()) {
                            std::string text = message["content"];
                            contentStream << text;
                            onChunk(text);
                        }

                        // 处理工具调用
                        if (message.contains("tool_calls")) {
                            for (const auto& callJson : message["tool_calls"]) {
                                ChatMessage::ToolCall call;
                                call.id = callJson.value("id", "");

                                if (callJson.contains("function")) {
                                    call.name = callJson["function"].value("name", "");
                                    std::string argsStr = callJson["function"].value("arguments", "{}");
                                    try {
                                        call.arguments = json::parse(argsStr);
                                    } catch (...) {
                                        call.arguments = json::object();
                                    }
                                }

                                accumulatedToolCalls.push_back(call);
                            }
                        }
                    }
                }
            } catch (...) {
                // JSON解析失败，可能是真正的SSE数据（未来实现）
                // 暂时忽略
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
    response.has_tool_calls = !accumulatedToolCalls.empty();
    response.tool_calls = accumulatedToolCalls;

    // 添加助手消息到历史（包含工具调用）
    ChatMessage assistantMsg(MessageRole::ASSISTANT, response.content);
    assistantMsg.tool_calls = accumulatedToolCalls;
    {
        std::unique_lock<std::shared_mutex> lock(history_mutex_);
        history_.push_back(assistantMsg);
    }

    // 如果有工具调用，执行工具
    if (response.has_tool_calls) {
        executeToolCalls(response.tool_calls);
    }

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

        // 构建 JSON Schema 格式的 parameters
        json paramsSchema;
        paramsSchema["type"] = "object";

        // 构建 properties
        json properties = json::object();
        std::vector<std::string> required;
        for (const auto& param : desc.parameters) {
            json paramSchema;
            paramSchema["type"] = param.type;
            paramSchema["description"] = param.description;

            // 添加默认值（如果有）
            if (!param.default_value.empty()) {
                if (param.type == "string") {
                    paramSchema["default"] = param.default_value;
                } else if (param.type == "integer") {
                    try {
                        paramSchema["default"] = std::stoi(param.default_value);
                    } catch (...) {
                        // 保持默认为字符串
                    }
                }
            }

            properties[param.name] = paramSchema;

            // 记录必需参数
            if (param.required) {
                required.push_back(param.name);
            }
        }
        paramsSchema["properties"] = properties;

        // 添加 required 数组（如果有必需参数）
        if (!required.empty()) {
            paramsSchema["required"] = required;
        }

        tool.input_schema = paramsSchema;
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
    if (toolCalls.empty()) {
        return true;
    }

    // 如果启用并发执行且有多个工具调用
    if (config_.concurrent_tool_execution && toolCalls.size() > 1 && thread_pool_) {
        return executeToolCallsConcurrent(toolCalls);
    }

    // 顺序执行
    return executeToolCallsSequential(toolCalls);
}

bool Agent::executeToolCallsSequential(const std::vector<ChatMessage::ToolCall>& toolCalls) {
    bool allSuccess = true;

    for (const auto& call : toolCalls) {
        // 执行工具
        ToolResult result = tool_executor_->execute(call.name, call.arguments);

        // 存储结果
        {
            std::lock_guard<std::mutex> lock(tool_results_mutex_);
            tool_results_[call.id] = result;
        }

        // 添加工具响应消息到历史
        ChatMessage toolMsg(MessageRole::TOOL, result.success ? result.content : result.error_message);
        toolMsg.tool_call_id = call.id;
        toolMsg.is_error = !result.success;
        {
            std::unique_lock<std::shared_mutex> lock(history_mutex_);
            history_.push_back(toolMsg);
        }

        if (!result.success) {
            allSuccess = false;
        }
    }

    return allSuccess;
}

bool Agent::executeToolCallsConcurrent(const std::vector<ChatMessage::ToolCall>& toolCalls) {
    std::vector<std::future<std::pair<std::string, ToolResult>>> futures;

    // 提交所有工具调用到线程池
    for (const auto& call : toolCalls) {
        auto future = thread_pool_->submitWithResult(
            [this, call]() -> std::pair<std::string, ToolResult> {
                ToolResult result = tool_executor_->execute(call.name, call.arguments);
                return std::make_pair(call.id, result);
            }
        );
        futures.push_back(std::move(future));
    }

    // 等待所有任务完成并收集结果
    bool allSuccess = true;
    for (auto& future : futures) {
        try {
            auto [callId, result] = future.get();

            // 存储结果
            {
                std::lock_guard<std::mutex> lock(tool_results_mutex_);
                tool_results_[callId] = result;
            }

            // 添加工具响应消息到历史
            ChatMessage toolMsg(MessageRole::TOOL, result.success ? result.content : result.error_message);
            toolMsg.tool_call_id = callId;
            toolMsg.is_error = !result.success;
            {
                std::unique_lock<std::shared_mutex> lock(history_mutex_);
                history_.push_back(toolMsg);
            }

            if (!result.success) {
                allSuccess = false;
            }
        } catch (const std::exception& e) {
            LOG_ERROR("并发工具执行异常: " + std::string(e.what()));
            allSuccess = false;
        }
    }

    return allSuccess;
}

std::vector<ChatMessage> Agent::buildMessages(const std::string& userMessage) {
    std::vector<ChatMessage> messages;

    // 获取所有历史消息（包含完整的对话历史）
    std::vector<ChatMessage> history;
    {
        std::shared_lock<std::shared_mutex> lock(history_mutex_);
        history = history_;  // 复制全部历史
    }

    // 如果启用了Token优化，压缩历史
    if (token_optimization_enabled_ && token_optimizer_) {
        messages = token_optimizer_->compressHistory(history, 8000);
    } else {
        messages = history;
    }

    return messages;
}

bool Agent::shouldContinue(const AgentResponse& response) const {
    // 如果有工具调用，需要继续
    return response.has_tool_calls;
}

void Agent::addToHistory(const ChatMessage& msg) {
    std::unique_lock<std::shared_mutex> lock(history_mutex_);
    history_.push_back(msg);
}

const std::vector<ChatMessage>& Agent::getHistory() const {
    // 返回引用需要调用者注意线程安全
    // 这里不返回锁，调用者需要自行管理
    return history_;
}

void Agent::clearHistory() {
    std::unique_lock<std::shared_mutex> lock(history_mutex_);
    history_.clear();
}

} // namespace roboclaw
