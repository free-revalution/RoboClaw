#include <gtest/gtest.h>
#include "social/social_message.h"
#include <nlohmann/json.hpp>

using namespace roboclaw::social;

TEST(SocialMessage, CanCreateMessage) {
    SocialMessage msg;
    msg.platform_id = "telegram";
    msg.chat_id = "chat123";
    msg.user_id = "user456";
    msg.content = "Hello";
    msg.timestamp = 1234567890;

    EXPECT_EQ(msg.platform_id, "telegram");
    EXPECT_EQ(msg.content, "Hello");
}

TEST(SocialMessage, CanConvertToJson) {
    SocialMessage msg;
    msg.platform_id = "telegram";
    msg.chat_id = "chat123";
    msg.content = "Test";

    json j = msg.toJson();
    EXPECT_EQ(j["platform_id"], "telegram");
    EXPECT_EQ(j["content"], "Test");
}

TEST(SocialMessage, CanConvertFromJson) {
    json j = {
        {"platform_id", "telegram"},
        {"chat_id", "chat123"},
        {"content", "Test"}
    };

    SocialMessage msg = SocialMessage::fromJson(j);
    EXPECT_EQ(msg.platform_id, "telegram");
    EXPECT_EQ(msg.content, "Test");
}
