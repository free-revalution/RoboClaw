// Agent核心类 - Agent
// 负责管理对话、调用LLM、执行工具

#ifndef ROBOCLAW_AGENT_AGENT_H
#define ROBOCLAW_AGENT_AGENT_H

#include "../llm/llm_provider.h"
#include "tool_executor.h"
#include "prompt_builder.h"
#include "task_coordinator.h"
#include "../optimization/token_optimizer.h"
#include "../optimization/token_budget.h"
#include "../utils/thread_pool.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <shared_mutex>
#include <mutex>

namespace roboclaw {

// Agent配置
struct AgentConfig {
    int max_iterations;           // 最大工具调用迭代次数
    int max_tokens;               // 最大输出token数
    double temperature;           // 温度参数
    bool stream_response;         // 是否流式响应
    bool concurrent_tool_execution; // 是否并发执行工具

    AgentConfig()
        : max_iterations(10)
        , max_tokens(4096)
        , temperature(0.0)
        , stream_response(false)
        , concurrent_tool_execution(false) {}  // 默认关闭以保证兼容性
};

// Agent响应
struct AgentResponse {
    std::string content;                  // 最终回复内容
    std::vector<ChatMessage::ToolCall> tool_calls;  // 工具调用
    bool has_tool_calls;                  // 是否有工具调用
    bool success;                         // 是否成功
    std::string error;                    // 错误信息

    // 使用量统计
    int total_input_tokens;
    int total_output_tokens;

    AgentResponse()
        : has_tool_calls(false)
        , success(false)
        , total_input_tokens(0)
        , total_output_tokens(0) {}
};

// Agent类
class Agent {
public:
    Agent(std::unique_ptr<LLMProvider> llmProvider,
          std::unique_ptr<ToolExecutor> toolExecutor);

    ~Agent() = default;

    // 设置配置
    void setConfig(const AgentConfig& config) { config_ = config; }
    AgentConfig getConfig() const { return config_; }

    // 设置提示词模式
    void setPromptMode(PromptMode mode) {
        prompt_builder_.setMode(mode);
    }

    // 处理用户消息（单次交互）
    AgentResponse process(const std::string& userMessage);

    // 处理用户消息（带历史记录）
    AgentResponse process(const std::string& userMessage,
                         const std::vector<ChatMessage>& history);

    // 流式处理用户消息
    bool processStream(const std::string& userMessage,
                      std::function<void(const std::string&)> onChunk,
                      std::function<void(const AgentResponse&)> onComplete);

    // 获取对话历史
    const std::vector<ChatMessage>& getHistory() const;

    // 清空历史
    void clearHistory();

    // 设置系统提示词
    void setSystemPrompt(const std::string& prompt) {
        prompt_builder_.setSystemPrompt(prompt);
    }

    // 添加消息到历史
    void addToHistory(const ChatMessage& msg);

    // 设置Token优化器
    void setTokenOptimizer(std::shared_ptr<TokenOptimizer> optimizer) {
        token_optimizer_ = optimizer;
    }

    // 设置Token预算管理
    void setTokenBudget(std::shared_ptr<TokenBudget> budget) {
        token_budget_ = budget;
    }

    // 启用Token优化
    void enableTokenOptimization(bool enable) {
        token_optimization_enabled_ = enable;
    }

    // 启用并发工具执行
    void enableConcurrentToolExecution(bool enable) {
        config_.concurrent_tool_execution = enable;
    }

    // 设置线程池
    void setThreadPool(std::shared_ptr<ThreadPool> pool) {
        thread_pool_ = pool;
    }

    // 设置任务协调器
    void setTaskCoordinator(std::shared_ptr<agent::TaskCoordinator> coordinator) {
        task_coordinator_ = coordinator;
    }

    // 获取任务协调器
    std::shared_ptr<agent::TaskCoordinator> getTaskCoordinator() const {
        return task_coordinator_;
    }

private:
    // 执行一轮对话
    AgentResponse performOneRound(const std::vector<ChatMessage>& messages);

    // 执行工具调用
    bool executeToolCalls(const std::vector<ChatMessage::ToolCall>& toolCalls);

    // 顺序执行工具调用
    bool executeToolCallsSequential(const std::vector<ChatMessage::ToolCall>& toolCalls);

    // 并发执行工具调用
    bool executeToolCallsConcurrent(const std::vector<ChatMessage::ToolCall>& toolCalls);

    // 构建消息列表
    std::vector<ChatMessage> buildMessages(const std::string& userMessage);

    // 检查是否需要继续迭代
    bool shouldContinue(const AgentResponse& response) const;

    // 成员变量
    std::unique_ptr<LLMProvider> llm_provider_;
    std::unique_ptr<ToolExecutor> tool_executor_;
    PromptBuilder prompt_builder_;
    AgentConfig config_;

    // Token优化
    std::shared_ptr<TokenOptimizer> token_optimizer_;
    std::shared_ptr<TokenBudget> token_budget_;
    bool token_optimization_enabled_;

    // 线程池
    std::shared_ptr<ThreadPool> thread_pool_;

    // 任务协调器
    std::shared_ptr<agent::TaskCoordinator> task_coordinator_;

    // 对话历史
    std::vector<ChatMessage> history_;

    // 工具执行结果
    std::map<std::string, ToolResult> tool_results_;

    // 线程安全保护
    mutable std::shared_mutex history_mutex_;
    std::mutex tool_results_mutex_;
};

} // namespace roboclaw

#endif // ROBOCLAW_AGENT_AGENT_H
