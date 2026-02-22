#pragma once

#include "social_message.h"
#include <vector>
#include <string>

namespace roboclaw::social {

class ISocialAdapter {
public:
    virtual ~ISocialAdapter() = default;

    // 连接管理
    virtual bool connect(const nlohmann::json& config) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;

    // 消息接收
    virtual std::vector<SocialMessage> receiveMessages() = 0;

    // 消息发送
    virtual bool sendMessage(const std::string& chat_id, const std::string& content) = 0;
    virtual bool sendFile(const std::string& chat_id, const std::string& file_path) = 0;

    // 命令处理
    virtual std::string getCommandPrefix() const = 0;
    virtual std::string getPlatformName() const = 0;
};

} // namespace roboclaw::social
