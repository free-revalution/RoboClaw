// Token优化器 - TokenOptimizer
// 提供token估算、压缩和缓存功能

#ifndef ROBOCLAW_OPTIMIZATION_TOKEN_OPTIMIZER_H
#define ROBOCLAW_OPTIMIZATION_TOKEN_OPTIMIZER_H

#include "../llm/llm_provider.h"
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <unordered_map>
#include <mutex>
#include <list>
#include <atomic>

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

    // 缓存配置
    bool enable_token_cache;
    size_t max_cache_size;

    TokenOptimizationConfig()
        : enable_compression(true)
        , compression_threshold(8000)
        , enable_prompt_caching(true)
        , compress_tool_results(true)
        , max_tool_result_length(5000)
        , target_budget(12000)
        , enable_token_cache(true)
        , max_cache_size(1000) {}
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

    // 清空缓存
    void clearCache();

    // 获取缓存统计
    size_t getCacheSize() const;
    size_t getCacheHits() const;
    size_t getCacheMisses() const;

private:
    TokenOptimizationConfig config_;
    TokenStats stats_;

    // Token估算算法
    int estimateTokensOptimized(const std::string& text) const;
    int estimateTokensImpl(const std::vector<ChatMessage>& messages) const;

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

    // Token估算缓存条目
    struct CacheEntry {
        int token_count;
        size_t access_count;

        CacheEntry(int tokens) : token_count(tokens), access_count(1) {}
    };

    // 缓存键类型
    using CacheKey = std::string;

    // 生成缓存键
    CacheKey generateCacheKey(const std::vector<ChatMessage>& messages) const;
    CacheKey generateCacheKey(const std::string& text) const;

    // 缓存存储（LRU）
    mutable std::unordered_map<CacheKey, std::list<CacheEntry>::iterator> cache_index_;
    mutable std::list<CacheEntry> cache_list_;

    // 缓存统计
    mutable std::atomic<size_t> cache_hits_;
    mutable std::atomic<size_t> cache_misses_;

    // 缓存互斥锁
    mutable std::mutex cache_mutex_;
};

} // namespace roboclaw

#endif // ROBOCLAW_OPTIMIZATION_TOKEN_OPTIMIZER_H
