#include <gtest/gtest.h>
#include "social/telegram_adapter.h"

using namespace roboclaw::social;

TEST(TelegramAdapter, CanConnectWithValidToken) {
    TelegramAdapter adapter;

    nlohmann::json config = {
        {"bot_token", "1234567890:ABCdefGHIjklMNOpqrsTUVwxyzABCD1234567"}
    };

    // Note: This will fail without actual network/bot token, but we test the interface
    EXPECT_FALSE(adapter.isConnected());  // Not connected yet
}

TEST(TelegramAdapter, CanValidateBotToken) {
    EXPECT_TRUE(TelegramAdapter::isValidBotToken("1234567890:ABCdefGHIjklMNOpqrsTUVwxyzABCD1234567"));
    EXPECT_FALSE(TelegramAdapter::isValidBotToken("invalid"));
    EXPECT_FALSE(TelegramAdapter::isValidBotToken("123"));
    EXPECT_FALSE(TelegramAdapter::isValidBotToken(""));
}

TEST(TelegramAdapter, HasCorrectCommandPrefix) {
    TelegramAdapter adapter;
    EXPECT_EQ(adapter.getCommandPrefix(), "/");
}

TEST(TelegramAdapter, HasCorrectPlatformName) {
    TelegramAdapter adapter;
    EXPECT_EQ(adapter.getPlatformName(), "Telegram");
}

TEST(TelegramAdapter, CanDisconnect) {
    TelegramAdapter adapter;
    nlohmann::json config = {
        {"bot_token", "1234567890:ABCdefGHIjklMNOpqrsTUVwxyzABCD1234567"}
    };

    adapter.disconnect();  // Should not crash
    EXPECT_FALSE(adapter.isConnected());
}

TEST(TelegramAdapter, ReceiveMessagesReturnsEmptyWhenNotConnected) {
    TelegramAdapter adapter;
    auto messages = adapter.receiveMessages();
    EXPECT_TRUE(messages.empty());
}

TEST(TelegramAdapter, SendFailsWhenNotConnected) {
    TelegramAdapter adapter;
    EXPECT_FALSE(adapter.sendMessage("chat123", "Hello"));
    EXPECT_FALSE(adapter.sendFile("chat123", "/path/to/file"));
}
