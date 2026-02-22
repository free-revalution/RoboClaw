#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <chrono>
#include "social/social_adapter.h"
#include "social/social_message.h"
#include "social/social_manager.h"

using namespace roboclaw::social;

// Mock adapter for testing
class MockSocialAdapter : public ISocialAdapter {
public:
    MockSocialAdapter(const std::string& platform_id,
                      const std::string& command_prefix = "/")
        : platform_id_(platform_id),
          command_prefix_(command_prefix),
          connected_(false) {}

    bool connect(const nlohmann::json& config) override {
        connected_ = true;
        return true;
    }

    void disconnect() override {
        connected_ = false;
    }

    bool isConnected() const override {
        return connected_;
    }

    std::vector<SocialMessage> receiveMessages() const override {
        std::lock_guard<std::mutex> lock(messages_mutex_);
        std::vector<SocialMessage> result;
        for (const auto& msg : pending_messages_) {
            result.push_back(msg);
        }
        pending_messages_.clear();
        return result;
    }

    bool sendMessage(const std::string& chat_id, const std::string& content) override {
        sent_messages_.push_back({chat_id, content});
        return true;
    }

    bool sendFile(const std::string& chat_id, const std::string& file_path) override {
        sent_files_.push_back({chat_id, file_path});
        return true;
    }

    std::string getCommandPrefix() const override {
        return command_prefix_;
    }

    std::string getPlatformName() const override {
        return platform_id_;
    }

    // Test helpers
    void addPendingMessage(const SocialMessage& msg) {
        std::lock_guard<std::mutex> lock(messages_mutex_);
        pending_messages_.push_back(msg);
    }

    size_t getSentMessageCount() const {
        return sent_messages_.size();
    }

    std::string getLastSentContent() const {
        if (sent_messages_.empty()) return "";
        return sent_messages_.back().second;
    }

    std::string getLastSentChatId() const {
        if (sent_messages_.empty()) return "";
        return sent_messages_.back().first;
    }

    void clearSentMessages() {
        sent_messages_.clear();
    }

private:
    std::string platform_id_;
    std::string command_prefix_;
    bool connected_;

    mutable std::mutex messages_mutex_;
    std::vector<SocialMessage> pending_messages_;
    mutable std::vector<std::pair<std::string, std::string>> sent_messages_;
    std::vector<std::pair<std::string, std::string>> sent_files_;
};

class SocialManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager_ = std::make_unique<SocialManager>();

        // Create mock adapters
        telegram_adapter_ = std::make_shared<MockSocialAdapter>("telegram", "/");
        wechat_adapter_ = std::make_shared<MockSocialAdapter>("wechat", "!");
    }

    void TearDown() override {
        if (manager_ && manager_->isMessageLoopRunning()) {
            manager_->stopMessageLoop();
        }
    }

    std::unique_ptr<SocialManager> manager_;
    std::shared_ptr<MockSocialAdapter> telegram_adapter_;
    std::shared_ptr<MockSocialAdapter> wechat_adapter_;
};

TEST_F(SocialManagerTest, RegisterAdapter) {
    manager_->registerAdapter("telegram", telegram_adapter_);

    EXPECT_EQ(manager_->getAdapterCount(), 1);
}

TEST_F(SocialManagerTest, RegisterMultipleAdapters) {
    manager_->registerAdapter("telegram", telegram_adapter_);
    manager_->registerAdapter("wechat", wechat_adapter_);

    EXPECT_EQ(manager_->getAdapterCount(), 2);
}

TEST_F(SocialManagerTest, ConnectPlatform) {
    manager_->registerAdapter("telegram", telegram_adapter_);

    nlohmann::json config = {{"bot_token", "test_token"}};

    bool connected = manager_->connectPlatform("telegram", config);

    EXPECT_TRUE(connected);
    EXPECT_TRUE(manager_->isPlatformConnected("telegram"));
}

TEST_F(SocialManagerTest, ConnectUnknownPlatform) {
    nlohmann::json config = {{"bot_token", "test_token"}};

    bool connected = manager_->connectPlatform("unknown", config);

    EXPECT_FALSE(connected);
    EXPECT_FALSE(manager_->isPlatformConnected("unknown"));
}

TEST_F(SocialManagerTest, DisconnectPlatform) {
    manager_->registerAdapter("telegram", telegram_adapter_);

    nlohmann::json config = {{"bot_token", "test_token"}};
    manager_->connectPlatform("telegram", config);

    EXPECT_TRUE(manager_->isPlatformConnected("telegram"));

    manager_->disconnectPlatform("telegram");

    EXPECT_FALSE(manager_->isPlatformConnected("telegram"));
}

TEST_F(SocialManagerTest, ProcessSimpleMessage) {
    manager_->registerAdapter("telegram", telegram_adapter_);

    nlohmann::json config = {{"bot_token", "test_token"}};
    manager_->connectPlatform("telegram", config);

    SocialMessage msg;
    msg.platform_id = "telegram";
    msg.chat_id = "test_chat";
    msg.user_id = "user123";
    msg.content = "Hello, RoboClaw!";
    msg.message_id = "msg001";
    msg.timestamp = 1234567890;
    msg.metadata = {{"test", "data"}};

    bool processed = manager_->processMessage(msg);

    EXPECT_TRUE(processed);
    EXPECT_GT(telegram_adapter_->getSentMessageCount(), 0);
}

TEST_F(SocialManagerTest, ProcessCommandMessage) {
    manager_->registerAdapter("telegram", telegram_adapter_);

    nlohmann::json config = {{"bot_token", "test_token"}};
    manager_->connectPlatform("telegram", config);

    SocialMessage msg;
    msg.platform_id = "telegram";
    msg.chat_id = "test_chat";
    msg.user_id = "user123";
    msg.content = "/help";
    msg.message_id = "msg002";
    msg.timestamp = 1234567891;
    msg.metadata = {};

    bool processed = manager_->processMessage(msg);

    EXPECT_TRUE(processed);
    EXPECT_GT(telegram_adapter_->getSentMessageCount(), 0);
}

TEST_F(SocialManagerTest, SendMessageThroughManager) {
    manager_->registerAdapter("telegram", telegram_adapter_);

    nlohmann::json config = {{"bot_token", "test_token"}};
    manager_->connectPlatform("telegram", config);

    bool sent = manager_->sendMessage("telegram", "test_chat", "Test response");

    EXPECT_TRUE(sent);
    EXPECT_EQ(telegram_adapter_->getSentMessageCount(), 1);
    EXPECT_EQ(telegram_adapter_->getLastSentChatId(), "test_chat");
    EXPECT_EQ(telegram_adapter_->getLastSentContent(), "Test response");
}

TEST_F(SocialManagerTest, SendMessageToDisconnectedPlatform) {
    manager_->registerAdapter("telegram", telegram_adapter_);

    // Don't connect
    bool sent = manager_->sendMessage("telegram", "test_chat", "Test response");

    EXPECT_FALSE(sent);
    EXPECT_EQ(telegram_adapter_->getSentMessageCount(), 0);
}

TEST_F(SocialManagerTest, MessageLoopStartsAndStops) {
    manager_->registerAdapter("telegram", telegram_adapter_);

    nlohmann::json config = {{"bot_token", "test_token"}};
    manager_->connectPlatform("telegram", config);

    EXPECT_FALSE(manager_->isMessageLoopRunning());

    manager_->startMessageLoop();

    EXPECT_TRUE(manager_->isMessageLoopRunning());

    // Give the loop a moment to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    manager_->stopMessageLoop();

    EXPECT_FALSE(manager_->isMessageLoopRunning());
}

TEST_F(SocialManagerTest, MessageLoopProcessesPendingMessages) {
    manager_->registerAdapter("telegram", telegram_adapter_);

    nlohmann::json config = {{"bot_token", "test_token"}};
    manager_->connectPlatform("telegram", config);

    // Add a pending message to the mock adapter
    SocialMessage msg;
    msg.platform_id = "telegram";
    msg.chat_id = "test_chat";
    msg.user_id = "user123";
    msg.content = "Test message from loop";
    msg.message_id = "msg003";
    msg.timestamp = 1234567892;
    msg.metadata = {};

    telegram_adapter_->addPendingMessage(msg);

    manager_->startMessageLoop();

    // Give the loop time to process the message
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    manager_->stopMessageLoop();

    // The message should have been processed
    EXPECT_GT(telegram_adapter_->getSentMessageCount(), 0);
}

TEST_F(SocialManagerTest, MultiplePlatformsIndependent) {
    manager_->registerAdapter("telegram", telegram_adapter_);
    manager_->registerAdapter("wechat", wechat_adapter_);

    nlohmann::json telegram_config = {{"bot_token", "telegram_token"}};
    nlohmann::json wechat_config = {{"app_id", "wechat_app_id"}};

    EXPECT_TRUE(manager_->connectPlatform("telegram", telegram_config));
    EXPECT_TRUE(manager_->connectPlatform("wechat", wechat_config));

    EXPECT_TRUE(manager_->isPlatformConnected("telegram"));
    EXPECT_TRUE(manager_->isPlatformConnected("wechat"));

    // Disconnect only telegram
    manager_->disconnectPlatform("telegram");

    EXPECT_FALSE(manager_->isPlatformConnected("telegram"));
    EXPECT_TRUE(manager_->isPlatformConnected("wechat"));
}

TEST_F(SocialManagerTest, ProcessMessageWithErrorHandling) {
    manager_->registerAdapter("telegram", telegram_adapter_);

    nlohmann::json config = {{"bot_token", "test_token"}};
    manager_->connectPlatform("telegram", config);

    SocialMessage msg;
    msg.platform_id = "telegram";
    msg.chat_id = "test_chat";
    msg.user_id = "user123";
    msg.content = "";  // Empty content - should still be processed
    msg.message_id = "msg004";
    msg.timestamp = 1234567893;
    msg.metadata = {};

    // Should handle gracefully and return result
    bool processed = manager_->processMessage(msg);

    // Even with empty content, should process (maybe with error response)
    EXPECT_TRUE(processed);
}

TEST_F(SocialManagerTest, StartMessageLoopTwiceIsSafe) {
    manager_->registerAdapter("telegram", telegram_adapter_);

    nlohmann::json config = {{"bot_token", "test_token"}};
    manager_->connectPlatform("telegram", config);

    manager_->startMessageLoop();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Second call should be safe (no-op or just return)
    manager_->startMessageLoop();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    EXPECT_TRUE(manager_->isMessageLoopRunning());

    manager_->stopMessageLoop();

    EXPECT_FALSE(manager_->isMessageLoopRunning());
}

TEST_F(SocialManagerTest, StopMessageLoopWhenNotRunningIsSafe) {
    manager_->registerAdapter("telegram", telegram_adapter_);

    // Stop when not started - should be safe
    EXPECT_NO_THROW({
        manager_->stopMessageLoop();
    });

    EXPECT_FALSE(manager_->isMessageLoopRunning());
}
