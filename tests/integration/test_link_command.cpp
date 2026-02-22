#include <gtest/gtest.h>
#include "cli/link_command.h"

using namespace roboclaw::cli;

TEST(LinkCommand, CanShowPlatformList) {
    LinkCommand cmd;

    auto platforms = cmd.getAvailablePlatforms();

    EXPECT_FALSE(platforms.empty());
    // Check if telegram is present
    bool has_telegram = false;
    for (const auto& p : platforms) {
        if (p.id == "telegram") has_telegram = true;
    }
    EXPECT_TRUE(has_telegram);
}

TEST(LinkCommand, CanValidateConfig) {
    LinkCommand cmd;

    nlohmann::json config = {
        {"bot_token", "1234567890:ABCdefGHIjklMNOpqrsTUVwxyz"}
    };

    EXPECT_TRUE(cmd.validatePlatformConfig("telegram", config));
}

TEST(LinkCommand, InvalidConfigRejected) {
    LinkCommand cmd;

    nlohmann::json config = {
        {"bot_token", "invalid"}
    };

    EXPECT_FALSE(cmd.validatePlatformConfig("telegram", config));
}
