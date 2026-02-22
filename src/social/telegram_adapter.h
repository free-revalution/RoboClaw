#pragma once

#include "social_adapter.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <cstdint>

namespace roboclaw::social {

class TelegramAdapter : public ISocialAdapter {
public:
    TelegramAdapter();
    ~TelegramAdapter() override;

    // ISocialAdapter 实现
    bool connect(const nlohmann::json& config) override;
    void disconnect() override;
    bool isConnected() const override;

    std::vector<SocialMessage> receiveMessages() const override;
    bool sendMessage(const std::string& chat_id, const std::string& content) override;
    bool sendFile(const std::string& chat_id, const std::string& file_path) override;

    std::string getCommandPrefix() const override { return "/"; }
    std::string getPlatformName() const override { return "Telegram"; }

    // Telegram 特定方法
    static bool isValidBotToken(const std::string& token);
    nlohmann::json getMe();

private:
    std::string bot_token_;
    std::string api_url_;
    bool connected_;
    mutable int last_update_id_;  // mutable for const receiveMessages

    std::string buildApiUrl(const std::string& method) const;
    nlohmann::json httpGet(const std::string& url) const;
    nlohmann::json httpPost(const std::string& url, const nlohmann::json& data) const;
};

} // namespace roboclaw::social
