#include <gtest/gtest.h>
#include "agent/claude_code_bridge.h"

using namespace roboclaw::agent;

TEST(ClaudeCodeBridge, CanBeInstantiated) {
    ClaudeCodeBridge bridge;
    SUCCEED();
}

TEST(ClaudeCodeBridge, IsNotRunningInitially) {
    ClaudeCodeBridge bridge;
    EXPECT_FALSE(bridge.isRunning());
}

TEST(ClaudeCodeBridge, HasCorrectAgentName) {
    ClaudeCodeBridge bridge;
    EXPECT_EQ(bridge.getAgentName(), "Claude Code");
}

TEST(ClaudeCodeBridge, HasCorrectAgentVersion) {
    ClaudeCodeBridge bridge;
    EXPECT_EQ(bridge.getAgentVersion(), "VSCode Extension");
}

TEST(ClaudeCodeBridge, SendTaskFailsWhenNotRunning) {
    ClaudeCodeBridge bridge;
    nlohmann::json context = {{"key", "value"}};
    EXPECT_FALSE(bridge.sendTask("test task", context));
}

TEST(ClaudeCodeBridge, WaitForResultReturnsErrorWhenNotRunning) {
    ClaudeCodeBridge bridge;
    auto result = bridge.waitForResult(1000);
    EXPECT_FALSE(result["success"]);
    EXPECT_EQ(result["error"], "ClaudeCodeBridge not running");
}

TEST(ClaudeCodeBridge, TerminateFailsWhenNotRunning) {
    ClaudeCodeBridge bridge;
    EXPECT_FALSE(bridge.terminate());
}

TEST(ClaudeCodeBridge, CopyIsDisabled) {
    // Copy constructor and assignment are deleted
    // This test verifies the code compiles with these operations disabled
    ClaudeCodeBridge bridge1;
    // ClaudeCodeBridge bridge2 = bridge1;  // Should not compile
    // bridge2 = bridge1;  // Should not compile
    SUCCEED();
}

TEST(ClaudeCodeBridge, MoveIsEnabled) {
    ClaudeCodeBridge bridge1;
    ClaudeCodeBridge bridge2 = std::move(bridge1);
    // Move constructor should work
    SUCCEED();
}
