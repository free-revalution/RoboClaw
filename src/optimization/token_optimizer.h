// Token优化器 - TokenOptimizer
// 提供token估算、压缩和缓存功能

#ifndef ROBOCLAW_OPTIMIZATION_TOKEN_OPTIMIZER_H
#define ROBOCLAW_OPTIMIZATION_TOKEN_OPTIMIZER_H

#include "../llm/llm_provider.h"
#include <string>
#include <vector>
#include <map>
#include <functional>

namespace roboclaw {

// Token统计信息
struct TokenStats {
    int input_tokens;
    int output_tokens;
    int total_tokens;
    int estimated_next;

    TokenStats()
        : input_tokens(0), output_tokens(0), total_tokens(0), estimated_next(0) {}
};

// Token优化配置
struct TokenOptimizationConfig {
    bool enable_compression;
    int compression_threshold;
    bool enable_prompt_caching;
    bool compress_tool_results;
    int max_tool_result_length;
    int target_budget;

    TokenOptimizationConfig()
        : enable_compression(true)
        , compression_threshold(8000)
        , enable_prompt_caching(true)
        , compress_tool_results(true)
        , max_tool_result_length(5000)
        , target_budget(12000) {}
};

// Token优化器
class TokenOptimizer {
public:
    TokenOptimizer();
    ~TokenOptimizer() = default;

    // 设置配置
    void setConfig(const TokenOptimizationConfig& config);
    TokenOptimizationConfig getConfig() const { return config_; }

    // 估算消息的token数
    int estimateTokens(const std::vector<ChatMessage>& messages);
    int estimateTokens(const std::string& text);

    // 压缩对话历史
    std::vector<ChatMessage> compressHistory(
        const std::vector<ChatMessage>& history,
        int target_tokens = -1);

    // 压缩工具结果
    std::string compressToolResult(const std::string& result, const std::string& toolName);

    // 生成缓存头（Anthropic/OpenAI）
    std::map<std::string, std::string> generateCacheHeaders(
        const std::string& provider);

    // 检查是否需要压缩
    bool needsCompression(const std::vector<ChatMessage>& messages) const;

    // 获取当前统计
    TokenStats getStats() const { return stats_; }

    // 更新统计
    void updateStats(int input, int output);

    // 预估下次请求的token数
    int estimateNextRequest(const std::vector<ChatMessage>& messages,
                           const std::vector<ToolDefinition>& tools) const;

    // 获取优化建议
    std::string getOptimizationSuggestion() const;

private:
    TokenOptimizationConfig config_;
    TokenStats stats_;

    // Token估算算法
    int estimateTokensOptimized(const std::string& text);

    // 简单估算（英文约4字符/token，中文约2字符/token）
    int estimateTokensSimple(const std::string& text);

    // 获取缓存的系统提示词key
    std::string getCachedPromptKey();

    // 分层压缩
    struct CompressionLayers {
        std::vector<ChatMessage> recent;      // 最近N条（完整）
        std::vector<ChatMessage> middle;      // 中期M条（摘要）
        std::vector<ChatMessage> old_summary;  // 远期（总摘要）
    };
    CompressionLayers createCompressionLayers(const std::vector<ChatMessage>& history);
};

} // namespace roboclaw

#endif // ROBOCLAW_OPTIMIZATION_TOKEN_OPTIMIZER_H
