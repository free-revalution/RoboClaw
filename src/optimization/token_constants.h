// Token优化常量定义
// Token optimization constants

#ifndef ROBOCLAW_OPTIMIZATION_TOKEN_CONSTANTS_H
#define ROBOCLAW_OPTIMIZATION_TOKEN_CONSTANTS_H

namespace roboclaw {
namespace TokenConstants {

    // Token估算常量
    constexpr int TOKENS_PER_TOOL_CALL = 50;       // 每个工具调用约50 tokens
    constexpr int CHARS_PER_TOKEN_ENGLISH = 4;      // 英文每token约4字符
    constexpr int CHARS_PER_TOKEN_CHINESE = 2;      // 中文每token约2字符
    constexpr int TOKENS_PER_MESSAGE_OVERHEAD = 10; // 每条消息额外开销

    // Token预算常量
    constexpr int DEFAULT_TOKEN_BUDGET = 12000;     // 默认token预算
    constexpr int MIN_TOKEN_BUDGET = 1000;          // 最小token预算
    constexpr int MAX_TOKEN_BUDGET = 200000;        // 最大token预算

    // 压缩阈值常量
    constexpr int DEFAULT_COMPRESSION_THRESHOLD = 8000;  // 默认压缩阈值
    constexpr int MIN_COMPRESSION_THRESHOLD = 1000;     // 最小压缩阈值
    constexpr int COMPRESSION_MESSAGE_LIMIT = 20;       // 超过20条消息开始压缩

    // 压缩层级常量
    constexpr int RECENT_MESSAGES_COUNT = 5;       // 保留完整最近消息数
    constexpr int MIDDLE_MESSAGES_COUNT = 10;      // 中期消息摘要数量
    constexpr int MAX_COMPRESSED_LENGTH = 100;     // 压缩内容最大长度
    constexpr int MAX_TOOL_CALLS_DISPLAY = 3;      // 最多显示的工具调用数

    // 警告级别阈值（百分比）
    constexpr double WARNING_THRESHOLD_LOW = 50.0;     // 50% - 低警告
    constexpr double WARNING_THRESHOLD_MEDIUM = 75.0;  // 75% - 中等警告
    constexpr double WARNING_THRESHOLD_HIGH = 90.0;    // 90% - 高警告
    constexpr double WARNING_THRESHOLD_CRITICAL = 100.0; // 100% - 严重警告

    // 工具结果压缩常量
    constexpr int DEFAULT_MAX_TOOL_RESULT_LENGTH = 5000;  // 默认最大工具结果长度
    constexpr int MIN_TOOL_RESULT_LENGTH = 100;           // 最小工具结果长度
    constexpr int MAX_TOOL_RESULT_LENGTH = 50000;         // 最大工具结果长度

    // 缓存常量
    constexpr int DEFAULT_PROMPT_CACHE_SIZE = 100;     // 默认提示词缓存大小(MB)
    constexpr int DEFAULT_SKILL_CACHE_TTL_HOURS = 168; // 默认技能缓存过期时间(7天)
    constexpr int MIN_CACHE_TTL_HOURS = 1;             // 最小缓存过期时间
    constexpr int MAX_CACHE_TTL_HOURS = 720;           // 最大缓存过期时间(30天)

    // 技能文件常量
    constexpr int MAX_SKILL_FILE_SIZE = 1024 * 1024;   // 最大技能文件大小(1MB)

    // URL常量
    constexpr int DEFAULT_HTTP_TIMEOUT_SECONDS = 30;   // 默认HTTP超时时间
    constexpr int MAX_HTTP_TIMEOUT_SECONDS = 300;      // 最大HTTP超时时间

} // namespace TokenConstants
} // namespace roboclaw

#endif // ROBOCLAW_OPTIMIZATION_TOKEN_CONSTANTS_H
