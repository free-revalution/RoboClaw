#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <cstdint>

namespace roboclaw::social {

struct SocialMessage {
    std::string platform_id;      // 平台标识
    std::string chat_id;          // 会话/群组 ID
    std::string user_id;          // 发送者 ID
    std::string content;          // 消息内容
    std::string message_id;       // 消息唯一 ID
    int64_t timestamp;           // 时间戳
    nlohmann::json metadata;      // 平台特定元数据

    nlohmann::json toJson() const {
        nlohmann::json j;
        j["platform_id"] = platform_id;
        j["chat_id"] = chat_id;
        j["user_id"] = user_id;
        j["content"] = content;
        j["message_id"] = message_id;
        j["timestamp"] = timestamp;
        j["metadata"] = metadata;
        return j;
    }

    static SocialMessage fromJson(const nlohmann::json& j) {
        SocialMessage msg;
        msg.platform_id = j.value("platform_id", "");
        msg.chat_id = j.value("chat_id", "");
        msg.user_id = j.value("user_id", "");
        msg.content = j.value("content", "");
        msg.message_id = j.value("message_id", "");
        msg.timestamp = j.value("timestamp", 0);
        msg.metadata = j.value("metadata", nlohmann::json{});
        return msg;
    }
};

} // namespace roboclaw::social
