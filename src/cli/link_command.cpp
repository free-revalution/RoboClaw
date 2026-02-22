#include "link_command.h"
#include <fstream>
#include <iostream>
#include "../social/telegram_adapter.h"

namespace roboclaw::cli {

LinkCommand::LinkCommand()
    : config_file_path_(".robopartner/social_config.json") {}

std::vector<PlatformInfo> LinkCommand::getAvailablePlatforms() const {
    std::vector<PlatformInfo> platforms;

    PlatformInfo telegram;
    telegram.id = "telegram";
    telegram.name = "Telegram";
    telegram.description = "Telegram Bot API";
    telegram.enabled = true;

    platforms.push_back(telegram);

    // TODO: Add other platforms (dingtalk, feishu)

    return platforms;
}

bool LinkCommand::validatePlatformConfig(const std::string& platform_id,
                                       const nlohmann::json& config) const {
    if (platform_id == "telegram") {
        return config.contains("bot_token") &&
               social::TelegramAdapter::isValidBotToken(config["bot_token"]);
    }
    return false;
}

bool LinkCommand::connectToPlatform(const std::string& platform_id,
                                   const nlohmann::json& config) {
    if (!validatePlatformConfig(platform_id, config)) {
        return false;
    }

    if (platform_id == "telegram") {
        social::TelegramAdapter adapter;
        if (adapter.connect(config)) {
            return saveConfig(platform_id, config);
        }
    }

    return false;
}

bool LinkCommand::disconnectPlatform(const std::string& platform_id) {
    // TODO: Implement disconnect logic
    return true;
}

std::string LinkCommand::getConnectionStatus() const {
    // TODO: Read config and return status
    return "No active connections";
}

bool LinkCommand::saveConfig(const std::string& platform_id,
                            const nlohmann::json& config) {
    // TODO: Implement config saving to .robopartner/social_config.json
    return true;
}

} // namespace roboclaw::cli
