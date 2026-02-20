// 对话压缩器 - ConversationCompressor
// 负责智能压缩对话历史

#ifndef ROBOCLAW_OPTIMIZATION_CONVERSATION_COMPRESSOR_H
#define ROBOCLAW_OPTIMIZATION_CONVERSATION_COMPRESSOR_H

#include "../llm/llm_provider.h"
#include <string>
#include <vector>
#include <memory>

namespace roboclaw {

// 对话压缩器
class ConversationCompressor {
public:
    ConversationCompressor();
    ~ConversationCompressor() = default;

    // 设置LLM提供商（用于生成摘要）
    void setLLMProvider(std::shared_ptr<LLMProvider> provider) {
        llm_provider_ = provider;
    }

    // 压缩分层结构
    struct CompressionLayers {
        std::vector<ChatMessage> recent;      // 最近N条（完整）
        std::vector<ChatMessage> middle;      // 中期M条（摘要）
        std::vector<ChatMessage> old_summary;  // 远期（总摘要）
    };

    // 执行压缩
    CompressionLayers compress(const std::vector<ChatMessage>& history,
                               int max_recent = 5,
                               int max_middle = 10);

    // 生成摘要
    std::string generateSummary(const std::vector<ChatMessage>& messages,
                               int max_length = 500);

    // 检查是否需要压缩
    bool needsCompression(const std::vector<ChatMessage>& history, int threshold) const;

private:
    std::shared_ptr<LLMProvider> llm_provider_;

    // 简化摘要生成（不调用LLM）
    std::string generateSimpleSummary(const std::vector<ChatMessage>& messages);

    // 从消息中提取关键信息
    std::string extractKeyInfo(const ChatMessage& msg) const;

    // 压缩助手消息
    ChatMessage compressAssistantMessage(const ChatMessage& msg);

    // 判断消息是否重要
    bool isImportantMessage(const ChatMessage& msg) const;
};

} // namespace roboclaw

#endif // ROBOCLAW_OPTIMIZATION_CONVERSATION_COMPRESSOR_H
