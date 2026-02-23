#include "telegram_adapter.h"
#include "utils/logger.h"
#include "../utils/code_quality_constants.h"
#include <stdexcept>
#include <regex>
#include <json/json.h>

namespace roboclaw::social {

TelegramAdapter::TelegramAdapter()
    : api_url_("https://api.telegram.org"), connected_(false), last_update_id_(0) {}

TelegramAdapter::~TelegramAdapter() {
    disconnect();
}

bool TelegramAdapter::connect(const nlohmann::json& config) {
    if (!config.contains("bot_token")) {
        LOG_ERROR("Telegram config missing bot_token");
        return false;
    }

    try {
        bot_token_ = config["bot_token"];
    } catch (const nlohmann::json::type_error& e) {
        LOG_ERROR("Invalid bot_token type in config: " + std::string(e.what()));
        return false;
    }

    if (!isValidBotToken(bot_token_)) {
        LOG_ERROR("Invalid Telegram bot token format");
        return false;
    }

    // 验证 token 通过调用 getMe
    try {
        nlohmann::json response = getMe();
        if (response.contains("ok") && response["ok"]) {
            connected_ = true;
            LOG_INFO("Telegram bot connected successfully");
            return true;
        } else {
            LOG_ERROR("Telegram bot authentication failed");
            return false;
        }
    } catch (const nlohmann::json::exception& e) {
        LOG_ERROR("Telegram JSON error during connection: " + std::string(e.what()));
        return false;
    } catch (const std::exception& e) {
        LOG_ERROR("Telegram connection error: " + std::string(e.what()));
        return false;
    }
}

void TelegramAdapter::disconnect() {
    connected_ = false;
    bot_token_.clear();
    last_update_id_ = 0;
    LOG_INFO("Telegram adapter disconnected");
}

bool TelegramAdapter::isConnected() const {
    return connected_;
}

std::vector<SocialMessage> TelegramAdapter::receiveMessages() const {
    std::vector<SocialMessage> messages;

    if (!connected_) {
        LOG_WARNING("Cannot receive messages: Telegram adapter not connected");
        return messages;
    }

    try {
        std::string url = buildApiUrl("getUpdates");
        url += "?offset=" + std::to_string(last_update_id_ + 1);
        url += "&timeout=" + std::to_string(
            std::chrono::duration_cast<std::chrono::seconds>(
                constants::LONG_POLL_TIMEOUT_MS
            ).count()
        );

        nlohmann::json response = httpGet(url);

        if (response.contains("ok") && response["ok"]) {
            nlohmann::json result = response["result"];
            for (const auto& update : result) {
                if (update.contains("message")) {
                    nlohmann::json msg = update["message"];
                    SocialMessage social_msg;
                    social_msg.platform_id = "telegram";
                    
                    try {
                        social_msg.chat_id = std::to_string(msg["chat"]["id"].get<int64_t>());
                        social_msg.user_id = std::to_string(msg["from"]["id"].get<int64_t>());
                        social_msg.content = msg.value("text", "");
                        social_msg.message_id = std::to_string(msg["message_id"].get<int64_t>());
                        social_msg.timestamp = msg.value("date", 0);

                        // Store raw update in metadata
                        social_msg.metadata = update;

                        last_update_id_ = update["update_id"];
                        messages.push_back(social_msg);
                    } catch (const nlohmann::json::exception& e) {
                        LOG_ERROR("Error parsing Telegram message: " + std::string(e.what()));
                        // Skip this message and continue with others
                        continue;
                    }
                }
            }
        } else if (response.contains("description")) {
            LOG_ERROR("Telegram API error: " + response["description"].get<std::string>());
        }
    } catch (const nlohmann::json::exception& e) {
        LOG_ERROR("Telegram JSON parse error in receiveMessages: " + std::string(e.what()));
    } catch (const std::exception& e) {
        LOG_ERROR("Telegram receiveMessages error: " + std::string(e.what()));
    }

    return messages;
}

bool TelegramAdapter::sendMessage(const std::string& chat_id, const std::string& content) {
    if (!connected_) {
        LOG_ERROR("Cannot send message: Telegram adapter not connected");
        return false;
    }

    if (chat_id.empty()) {
        LOG_ERROR("Cannot send message: chat_id is empty");
        return false;
    }

    if (content.empty()) {
        LOG_WARNING("Attempted to send empty message to chat_id: " + chat_id);
        return false;
    }

    try {
        std::string url = buildApiUrl("sendMessage");
        nlohmann::json payload = {
            {"chat_id", chat_id},
            {"text", content}
        };

        nlohmann::json response = httpPost(url, payload);
        
        if (response.contains("ok") && response["ok"]) {
            return true;
        } else if (response.contains("description")) {
            LOG_ERROR("Telegram send message failed: " + response["description"].get<std::string>());
            return false;
        }
        return false;
    } catch (const nlohmann::json::exception& e) {
        LOG_ERROR("Telegram JSON error in sendMessage: " + std::string(e.what()));
        return false;
    } catch (const std::exception& e) {
        LOG_ERROR("Telegram sendMessage error: " + std::string(e.what()));
        return false;
    }
}

bool TelegramAdapter::sendFile(const std::string& chat_id, const std::string& file_path) {
    if (!connected_) {
        LOG_ERROR("Cannot send file: Telegram adapter not connected");
        return false;
    }

    // TODO: 实现文件发送（需要 multipart/form-data）
    // This requires cpr's Multipart support
    LOG_WARNING("Telegram sendFile not yet implemented for file: " + file_path);
    return false;
}

bool TelegramAdapter::isValidBotToken(const std::string& token) {
    // Telegram Bot Token 格式: botid:hash
    // 格式: 1234567890:ABCdefGHIjklMNOpqrsTUVwxyz
    // Bot ID: 1-10 digits, followed by colon, followed by 35 characters of [A-Za-z0-9_-]
    try {
        std::regex pattern("^\\d+:[A-Za-z0-9_-]{35}$");
        return std::regex_match(token, pattern);
    } catch (const std::regex_error& e) {
        LOG_ERROR("Regex error validating bot token: " + std::string(e.what()));
        return false;
    }
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
