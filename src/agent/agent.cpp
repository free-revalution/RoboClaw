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

namespace roboclaw {

Agent::Agent(std::unique_ptr<LLMProvider> llmProvider,
             std::unique_ptr<ToolExecutor> toolExecutor)
    : llm_provider_(std::move(llmProvider))
    , tool_executor_(std::move(toolExecutor))
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
        // 手动构建 parameters JSON
        json paramsJson = json::array();
        for (const auto& param : desc.parameters) {
            paramsJson.push_back(param.toJson());
        }
        tool.input_schema = paramsJson;
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
    {
        std::unique_lock<std::shared_mutex> lock(history_mutex_);
        history_.push_back(assistantMsg);
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
        // 手动构建 parameters JSON
        json paramsJson = json::array();
        for (const auto& param : desc.parameters) {
            paramsJson.push_back(param.toJson());
        }
        tool.input_schema = paramsJson;
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

    // 获取历史消息（排除最后的用户消息）
    std::vector<ChatMessage> history;
    {
        std::shared_lock<std::shared_mutex> lock(history_mutex_);
        for (size_t i = 0; i < history_.size() - 1; ++i) {
            history.push_back(history_[i]);
        }
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
