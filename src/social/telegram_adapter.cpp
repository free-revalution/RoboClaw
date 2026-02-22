#include "telegram_adapter.h"
#include "utils/logger.h"
#include <stdexcept>
#include <regex>

namespace roboclaw::social {

TelegramAdapter::TelegramAdapter()
    : api_url_("https://api.telegram.org"), connected_(false), last_update_id_(0) {}

TelegramAdapter::~TelegramAdapter() {
    disconnect();
}

bool TelegramAdapter::connect(const nlohmann::json& config) {
    if (!config.contains("bot_token")) {
        return false;
    }

    bot_token_ = config["bot_token"];

    if (!isValidBotToken(bot_token_)) {
        return false;
    }

    // 验证 token 通过调用 getMe
    try {
        nlohmann::json response = getMe();
        if (response.contains("ok") && response["ok"]) {
            connected_ = true;
            return true;
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Telegram operation failed: " + std::string(e.what()));
        return false;
    } catch (...) {
        LOG_ERROR("Telegram operation failed: unknown error");
        return false;
    }

    return false;
}

void TelegramAdapter::disconnect() {
    connected_ = false;
    bot_token_.clear();
    last_update_id_ = 0;
}

bool TelegramAdapter::isConnected() const {
    return connected_;
}

std::vector<SocialMessage> TelegramAdapter::receiveMessages() const {
    std::vector<SocialMessage> messages;

    if (!connected_) {
        return messages;
    }

    try {
        std::string url = buildApiUrl("getUpdates");
        url += "?offset=" + std::to_string(last_update_id_ + 1);
        url += "&timeout=30";  // Long polling

        nlohmann::json response = httpGet(url);

        if (response.contains("ok") && response["ok"]) {
            nlohmann::json result = response["result"];
            for (const auto& update : result) {
                if (update.contains("message")) {
                    nlohmann::json msg = update["message"];
                    SocialMessage social_msg;
                    social_msg.platform_id = "telegram";
                    social_msg.chat_id = std::to_string(msg["chat"]["id"].get<int64_t>());
                    social_msg.user_id = std::to_string(msg["from"]["id"].get<int64_t>());
                    social_msg.content = msg.value("text", "");
                    social_msg.message_id = std::to_string(msg["message_id"].get<int64_t>());
                    social_msg.timestamp = msg.value("date", 0);

                    // Store raw update in metadata
                    social_msg.metadata = update;

                    last_update_id_ = update["update_id"];
                    messages.push_back(social_msg);
                }
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Telegram receiveMessages failed: " + std::string(e.what()));
    } catch (...) {
        LOG_ERROR("Telegram receiveMessages failed: unknown error");
    }

    return messages;
}

bool TelegramAdapter::sendMessage(const std::string& chat_id, const std::string& content) {
    if (!connected_) {
        return false;
    }

    try {
        std::string url = buildApiUrl("sendMessage");
        nlohmann::json payload = {
            {"chat_id", chat_id},
            {"text", content}
        };

        nlohmann::json response = httpPost(url, payload);
        return response.contains("ok") && response["ok"];
    } catch (const std::exception& e) {
        LOG_ERROR("Telegram sendMessage failed: " + std::string(e.what()));
        return false;
    } catch (...) {
        LOG_ERROR("Telegram sendMessage failed: unknown error");
        return false;
    }
}

bool TelegramAdapter::sendFile(const std::string& chat_id, const std::string& file_path) {
    if (!connected_) {
        return false;
    }

    // TODO: 实现文件发送（需要 multipart/form-data）
    // This requires cpr's Multipart support
    try {
        std::string url = buildApiUrl("sendDocument");
        // Placeholder implementation
        return false;
    } catch (const std::exception& e) {
        LOG_ERROR("Telegram sendFile failed: " + std::string(e.what()));
        return false;
    } catch (...) {
        LOG_ERROR("Telegram sendFile failed: unknown error");
        return false;
    }
}

bool TelegramAdapter::isValidBotToken(const std::string& token) {
    // Telegram Bot Token 格式: botid:hash
    // 格式: 1234567890:ABCdefGHIjklMNOpqrsTUVwxyz
    // Bot ID: 1-10 digits, followed by colon, followed by 35 characters of [A-Za-z0-9_-]
    std::regex pattern("^\\d+:[A-Za-z0-9_-]{35}$");
    return std::regex_match(token, pattern);
}

nlohmann::json TelegramAdapter::getMe() {
    return httpGet(buildApiUrl("getMe"));
}

std::string TelegramAdapter::buildApiUrl(const std::string& method) const {
    return api_url_ + "/bot" + bot_token_ + "/" + method;
}

nlohmann::json TelegramAdapter::httpGet(const std::string& url) const {
    // TODO: Implement actual HTTP GET using cpr
    // For now, return a mock response to allow compilation
    // This will be implemented with actual cpr calls
    nlohmann::json response;
    response["ok"] = false;
    response["description"] = "HTTP not implemented yet";
    return response;
}

nlohmann::json TelegramAdapter::httpPost(const std::string& url, const nlohmann::json& data) const {
    // TODO: Implement actual HTTP POST using cpr
    // For now, return a mock response to allow compilation
    // This will be implemented with actual cpr calls
    nlohmann::json response;
    response["ok"] = false;
    response["description"] = "HTTP not implemented yet";
    return response;
}

} // namespace roboclaw::social
