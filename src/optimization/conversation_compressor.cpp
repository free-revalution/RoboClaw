// ConversationCompressor实现

#include "conversation_compressor.h"
#include "../utils/logger.h"
#include <sstream>
#include <algorithm>

namespace roboclaw {

ConversationCompressor::ConversationCompressor() {
}

ConversationCompressor::CompressionLayers ConversationCompressor::compress(
        const std::vector<ChatMessage>& history,
        int max_recent,
        int max_middle) {

    CompressionLayers layers;

    if (history.empty()) {
        return layers;
    }

    size_t total = history.size();

    // 1. 最近消息（完整保留）
    size_t recent_count = std::min(static_cast<size_t>(max_recent), total);
    if (recent_count > 0) {
        auto start = history.begin() + (total - recent_count);
        layers.recent.insert(layers.recent.end(), start, history.end());
    }

    // 2. 中期消息（简化）
    size_t middle_count = std::min(static_cast<size_t>(max_middle), total - recent_count);
    if (middle_count > 0 && total > recent_count) {
        size_t middle_start = std::max(static_cast<size_t>(0), total - recent_count - middle_count);

        for (size_t i = middle_start; i < total - recent_count; ++i) {
            const auto& msg = history[i];

            if (msg.role == MessageRole::USER) {
                // 用户消息：保留完整内容
                layers.middle.push_back(msg);
            } else {
                // 助手消息：简化
                layers.middle.push_back(compressAssistantMessage(msg));
            }
        }
    }

    // 3. 远期消息：生成总摘要
    if (total > recent_count + middle_count) {
        std::vector<ChatMessage> old_messages(
            history.begin(),
            history.begin() + (total - recent_count - middle_count)
        );

        std::string summary = generateSimpleSummary(old_messages);

        if (!summary.empty()) {
            ChatMessage summary_msg(MessageRole::SYSTEM, summary);
            layers.old_summary.push_back(summary_msg);
        }
    }

    return layers;
}

std::string ConversationCompressor::generateSummary(
        const std::vector<ChatMessage>& messages,
        int max_length) {

    if (messages.empty()) {
        return "";
    }

    // 简化实现：生成文本摘要
    // 完整实现可以调用LLM生成更准确的摘要

    std::stringstream ss;
    ss << "[对话摘要] ";

    // 统计关键信息
    int user_messages = 0;
    int tool_calls = 0;
    std::vector<std::string> topics;

    for (const auto& msg : messages) {
        if (msg.role == MessageRole::USER) {
            user_messages++;
            // 提取关键词
            std::string key = extractKeyInfo(msg);
            if (!key.empty()) {
                topics.push_back(key);
            }
        } else if (!msg.tool_calls.empty()) {
            tool_calls += msg.tool_calls.size();
        }
    }

    ss << "包含" << user_messages << "轮用户对话";

    if (tool_calls > 0) {
        ss << "，使用了" << tool_calls << "次工具调用";
    }

    if (!topics.empty()) {
        ss << "。涉及主题：";
        for (size_t i = 0; i < topics.size() && i < 3; ++i) {
            if (i > 0) ss << "、";
            ss << topics[i];
        }
        }

    std::string summary = ss.str();

    // 限制长度
    if (summary.length() > static_cast<size_t>(max_length)) {
        summary = summary.substr(0, max_length - 3) + "...";
    }

    return summary;
}

bool ConversationCompressor::needsCompression(
        const std::vector<ChatMessage>& history,
        int threshold) const {

    if (history.size() > 20) {
        return true;  // 超过20条消息，建议压缩
    }

    // 估算token数
    int total_chars = 0;
    for (const auto& msg : history) {
        total_chars += msg.content.length();
    }

    // 粗略估算：约4字符/token
    int estimated_tokens = total_chars / 4;

    return estimated_tokens > threshold;
}

std::string ConversationCompressor::generateSimpleSummary(
        const std::vector<ChatMessage>& messages) {

    if (messages.empty()) {
        return "";
    }

    std::stringstream summary;
    summary << "[早期对话摘要] ";

    // 找到第一个用户消息作为主题
    std::string topic;
    for (const auto& msg : messages) {
        if (msg.role == MessageRole::USER) {
            topic = msg.content;
            if (topic.length() > 50) {
                topic = topic.substr(0, 47) + "...";
            }
            break;
        }
    }

    if (!topic.empty()) {
        summary << "对话主题：" << topic;
    }

    summary << " [" << messages.size() << "条消息]";

    return summary.str();
}

std::string ConversationCompressor::extractKeyInfo(const ChatMessage& msg) const {
    if (msg.role != MessageRole::USER) {
        return "";
    }

    std::string content = msg.content;
    std::string key;

    // 提取关键词（简化实现）
    // 查找常见的关键词模式
    std::vector<std::string> keywords = {
        "bug", "错误", "问题",
        "优化", "改进", "重构",
        "测试", "test",
        "文档", "document",
        "部署", "deploy"
    };

    for (const auto& keyword : keywords) {
        if (content.find(keyword) != std::string::npos) {
            key = keyword;
            break;
        }
    }

    return key;
}

ChatMessage ConversationCompressor::compressAssistantMessage(const ChatMessage& msg) {
    ChatMessage compressed;
    compressed.role = msg.role;

    if (!msg.tool_calls.empty()) {
        compressed.content = "[使用工具: ";
        for (size_t i = 0; i < msg.tool_calls.size(); ++i) {
            if (i > 0) compressed.content += ", ";
            compressed.content += msg.tool_calls[i].name;
        }
        compressed.content += "]";
    } else {
        // 普通消息：截断长内容
        std::string content = msg.content;
        if (content.length() > 200) {
            content = content.substr(0, 197) + "...";
        }
        compressed.content = content;
    }

    return compressed;
}

bool ConversationCompressor::isImportantMessage(const ChatMessage& msg) const {
    // 系统消息总是重要的
    if (msg.role == MessageRole::SYSTEM) {
        return true;
    }

    // 用户消息总是重要的
    if (msg.role == MessageRole::USER) {
        return true;
    }

    // 包含工具调用的助手消息是重要的
    if (!msg.tool_calls.empty()) {
        return true;
    }

    // 其他情况：根据内容判断
    return !msg.content.empty() && msg.content.length() < 100;
}

} // namespace roboclaw
