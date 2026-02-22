// TokenOptimizer实现

#include "token_optimizer.h"
#include "token_constants.h"
#include "../utils/logger.h"
#include <algorithm>
#include <sstream>
#include <regex>

namespace roboclaw {

TokenOptimizer::TokenOptimizer()
    : cache_hits_(0)
    , cache_misses_(0) {
    // 默认配置
}

void TokenOptimizer::setConfig(const TokenOptimizationConfig& config) {
    config_ = config;
}

int TokenOptimizer::estimateTokens(const std::vector<ChatMessage>& messages) {
    if (!config_.enable_token_cache) {
        return estimateTokensImpl(messages);
    }

    // 生成缓存键
    CacheKey key = generateCacheKey(messages);

    // 检查缓存
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        auto it = cache_index_.find(key);
        if (it != cache_index_.end()) {
            // 缓存命中，移动到LRU头部
            cache_hits_++;
            auto entry = it->second;
            cache_list_.splice(cache_list_.begin(), cache_list_, entry);
            return entry->token_count;
        }
    }

    // 缓存未命中
    cache_misses_++;
    int result = estimateTokensImpl(messages);

    // 添加到缓存
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        cache_list_.push_front(CacheEntry(result));
        cache_index_[key] = cache_list_.begin();

        // 如果超过最大缓存大小，移除最旧的条目
        while (cache_list_.size() > config_.max_cache_size) {
            auto last = cache_list_.end();
            --last;
            // 需要反向查找键（这是低效的，但简化了实现）
            for (auto it = cache_index_.begin(); it != cache_index_.end(); ++it) {
                if (it->second == last) {
                    cache_index_.erase(it);
                    break;
                }
            }
            cache_list_.pop_back();
        }
    }

    return result;
}

int TokenOptimizer::estimateTokensImpl(const std::vector<ChatMessage>& messages) const {
    int total = 0;

    for (const auto& msg : messages) {
        // 估算每条消息的token数
        total += estimateTokensOptimized(msg.content);

        // 工具调用也需要token
        if (!msg.tool_calls.empty()) {
            total += static_cast<int>(msg.tool_calls.size()) * TokenConstants::TOKENS_PER_TOOL_CALL;
        }
    }

    return total;
}

int TokenOptimizer::estimateTokens(const std::string& text) {
    if (text.empty()) {
        return 0;
    }

    if (!config_.enable_token_cache) {
        return estimateTokensOptimized(text);
    }

    // 生成缓存键
    CacheKey key = generateCacheKey(text);

    // 检查缓存
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        auto it = cache_index_.find(key);
        if (it != cache_index_.end()) {
            cache_hits_++;
            auto entry = it->second;
            cache_list_.splice(cache_list_.begin(), cache_list_, entry);
            return entry->token_count;
        }
    }

    // 缓存未命中
    cache_misses_++;
    int result = estimateTokensOptimized(text);

    // 添加到缓存
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        cache_list_.push_front(CacheEntry(result));
        cache_index_[key] = cache_list_.begin();

        while (cache_list_.size() > config_.max_cache_size) {
            auto last = cache_list_.end();
            --last;
            for (auto it = cache_index_.begin(); it != cache_index_.end(); ++it) {
                if (it->second == last) {
                    cache_index_.erase(it);
                    break;
                }
            }
            cache_list_.pop_back();
        }
    }

    return result;
}

int TokenOptimizer::estimateTokensOptimized(const std::string& text) const {
    // 使用混合估算策略
    int chinese_chars = 0;
    int english_chars = 0;
    int code_chars = 0;
    int whitespace = 0;

    for (char c : text) {
        if (c >= 0 && c <= 127) {
            // ASCII字符
            if (std::isspace(c)) {
                whitespace++;
            } else {
                english_chars++;
            }
        } else {
            // 非ASCII（主要是中文）
            chinese_chars++;
        }
    }

    // 估算规则：
    // 中文：约1.5字符/token
    // 英文：约4字符/token (CHARS_PER_TOKEN_ENGLISH)
    // 空格：约10字符/token
    // 代码：约4字符/token（但包含较多符号，保守估计）

    int tokens = 0;
    tokens += static_cast<int>(std::ceil(chinese_chars / 1.5));
    tokens += static_cast<int>(std::ceil(english_chars / static_cast<double>(TokenConstants::CHARS_PER_TOKEN_ENGLISH)));
    tokens += static_cast<int>(std::ceil(whitespace / 10.0));
    tokens += static_cast<int>(std::ceil(code_chars / static_cast<double>(TokenConstants::CHARS_PER_TOKEN_ENGLISH)));

    return std::max(tokens, 1);  // 至少1个token
}

std::vector<ChatMessage> TokenOptimizer::compressHistory(
        const std::vector<ChatMessage>& history,
        int target_tokens) {

    if (!config_.enable_compression) {
        return history;
    }

    int current_tokens = estimateTokensImpl(history);
    int threshold = (target_tokens > 0) ? target_tokens : config_.compression_threshold;

    if (current_tokens <= threshold) {
        return history;  // 不需要压缩
    }

    LOG_INFO("压缩对话历史: " + std::to_string(current_tokens) + " -> " + std::to_string(threshold) + " tokens");

    // 创建分层结构
    CompressionLayers layers = createCompressionLayers(history);

    // 重建消息列表
    std::vector<ChatMessage> compressed;

    // 1. 添加远期摘要（如果有）
    if (!layers.old_summary.empty()) {
        compressed.insert(compressed.end(), layers.old_summary.begin(), layers.old_summary.end());
    }

    // 2. 添加中期摘要（如果有）
    if (!layers.middle.empty()) {
        compressed.insert(compressed.end(), layers.middle.begin(), layers.middle.end());
    }

    // 3. 添加近期完整消息
    compressed.insert(compressed.end(), layers.recent.begin(), layers.recent.end());

    return compressed;
}

TokenOptimizer::CompressionLayers TokenOptimizer::createCompressionLayers(
        const std::vector<ChatMessage>& history) {

    CompressionLayers layers;

    size_t total = history.size();
    size_t recent_count = std::min(static_cast<size_t>(5), total);
    size_t middle_count = std::min(static_cast<size_t>(10), total - recent_count);

    // 最近消息（完整）
    if (recent_count > 0) {
        auto start = history.begin() + (total - recent_count);
        layers.recent.insert(layers.recent.end(), start, history.end());
    }

    // 中期消息（简化版：保留用户消息和工具消息，压缩助手回复）
    if (middle_count > 0 && total > recent_count) {
        size_t middle_start = std::max(static_cast<size_t>(0), total - recent_count - middle_count);
        for (size_t i = middle_start; i < total - recent_count; ++i) {
            const auto& msg = history[i];
            ChatMessage compressed_msg;

            if (msg.role == MessageRole::USER) {
                // 用户消息保留完整
                compressed_msg = msg;
            } else if (msg.role == MessageRole::TOOL) {
                // 工具消息必须保留完整（包含 tool_call_id），否则API会报错
                compressed_msg = msg;
            } else {
                // 助手消息只保留关键信息
                compressed_msg.role = msg.role;
                if (!msg.tool_calls.empty()) {
                    compressed_msg.content = "[使用了 " + std::to_string(msg.tool_calls.size()) + " 个工具]";
                } else {
                    // 截断长回复
                    std::string content = msg.content;
                    if (content.length() > 100) {
                        content = content.substr(0, 97) + "...";
                    }
                    compressed_msg.content = content;
                }
            }

            layers.middle.push_back(compressed_msg);
        }
    }

    // 远期消息：可以生成总摘要（这里简化为空，实际可调用LLM生成）
    // 如果有远期消息，这里可以添加一条总结消息

    return layers;
}

std::string TokenOptimizer::compressToolResult(const std::string& result, const std::string& toolName) {
    if (!config_.compress_tool_results) {
        return result;
    }

    std::string compressed = result;

    // 根据工具类型进行不同压缩
    if (toolName == "read") {
        // read工具：如果内容过长，只返回摘要
        if (compressed.length() > config_.max_tool_result_length) {
            compressed = compressed.substr(0, config_.max_tool_result_length - 50) + "\n... (文件较大，已截断)";
        }
    } else if (toolName == "bash") {
        // bash工具：只保留输出，移除错误堆栈的冗余信息
        // 简化实现：截断长输出
        if (compressed.length() > config_.max_tool_result_length) {
            compressed = compressed.substr(0, config_.max_tool_result_length) + "\n... (输出较长，已截断)";
        }
    }

    return compressed;
}

std::map<std::string, std::string> TokenOptimizer::generateCacheHeaders(
        const std::string& provider) {

    std::map<std::string, std::string> headers;

    if (!config_.enable_prompt_caching) {
        return headers;
    }

    // Anthropic缓存
    if (provider == "anthropic") {
        // 获取缓存的提示词key
        std::string cache_key = getCachedPromptKey();

        headers["anthropic-beta-prompt-caching"] = "enabled";
        headers["anthropic-beta-prompt-cache-header"] = "prompt_" + cache_key;
    }
    // OpenAI缓存
    else if (provider == "openai") {
        // OpenAI使用不同的缓存机制
        std::string cache_key = getCachedPromptKey();
        headers["x-cached-prompt"] = cache_key;
    }

    return headers;
}

bool TokenOptimizer::needsCompression(const std::vector<ChatMessage>& messages) const {
    if (!config_.enable_compression) {
        return false;
    }

    // 使用非缓存版本进行估算（const方法）
    int current_tokens = estimateTokensImpl(messages);
    return current_tokens > config_.compression_threshold;
}

void TokenOptimizer::updateStats(int input, int output) {
    stats_.input_tokens += input;
    stats_.output_tokens += output;
    stats_.total_tokens = stats_.input_tokens + stats_.output_tokens;
}

int TokenOptimizer::estimateNextRequest(
        const std::vector<ChatMessage>& messages,
        const std::vector<ToolDefinition>& tools) const {

    int tokens = estimateTokensImpl(messages);

    // 添加工具定义的token估算
    for (const auto& tool : tools) {
        // 估算描述的token数（不使用缓存）
        tokens += estimateTokensOptimized(tool.description);
        if (tool.input_schema.contains("properties")) {
            std::string schema_str = tool.input_schema.dump();
            tokens += estimateTokensOptimized(schema_str);
        }
    }

    // 添加系统提示词的token估算
    tokens += 500;  // 系统提示词约500 tokens

    return tokens;
}

std::string TokenOptimizer::getOptimizationSuggestion() const {
    if (stats_.total_tokens > config_.target_budget) {
        return "建议：已达到token预算，考虑启用对话压缩或开始新对话";
    }

    if (stats_.input_tokens > 10000) {
        return "建议：对话历史较长，启用压缩可以节省token";
    }

    if (!config_.enable_prompt_caching) {
        return "建议：启用提示词缓存可以节省90%的系统提示token";
    }

    return "当前token使用良好，无需优化";
}

std::string TokenOptimizer::getCachedPromptKey() {
    // 基于系统提示词内容生成缓存键
    std::string system_prompt = "你是RoboClaw，一个AI编程助手...";  // 使用固定的系统提示词

    // 简单哈希（实际应用中可以使用更好的哈希算法）
    std::hash<std::string> hash_fn;
    size_t hash_value = hash_fn(system_prompt);

    // 转换为十六进制字符串
    std::stringstream ss;
    ss << std::hex << hash_value;
    return ss.str();
}

// ==================== 缓存相关方法 ====================

void TokenOptimizer::clearCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_list_.clear();
    cache_index_.clear();
    cache_hits_ = 0;
    cache_misses_ = 0;
    LOG_INFO("Token估算缓存已清空");
}

size_t TokenOptimizer::getCacheSize() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return cache_list_.size();
}

size_t TokenOptimizer::getCacheHits() const {
    return cache_hits_.load();
}

size_t TokenOptimizer::getCacheMisses() const {
    return cache_misses_.load();
}

TokenOptimizer::CacheKey TokenOptimizer::generateCacheKey(const std::vector<ChatMessage>& messages) const {
    std::stringstream ss;

    // 生成消息序列的哈希键
    for (const auto& msg : messages) {
        ss << static_cast<int>(msg.role) << "|";

        // 使用内容的前100个字符作为键的一部分
        size_t len = std::min(size_t(100), msg.content.length());
        ss << msg.content.substr(0, len) << "|";

        // 包含工具调用信息
        ss << msg.tool_calls.size() << ";";
    }

    // 对长键进行哈希
    std::string key_str = ss.str();
    std::hash<std::string> hash_fn;
    size_t hash_value = hash_fn(key_str);

    std::stringstream result;
    result << std::hex << hash_value;
    return result.str();
}

TokenOptimizer::CacheKey TokenOptimizer::generateCacheKey(const std::string& text) const {
    // 对短文本直接使用前100字符，长文本使用哈希
    if (text.length() <= 100) {
        return text;
    }

    std::hash<std::string> hash_fn;
    size_t hash_value = hash_fn(text);

    std::stringstream ss;
    ss << std::hex << hash_value;
    return ss.str();
}

} // namespace roboclaw
