#pragma once

#include "agent_bridge.h"
#include <string>
#include <nlohmann/json.hpp>
#include <unistd.h>

namespace roboclaw::agent {

class ClaudeCodeBridge : public IAgentBridge {
public:
    ClaudeCodeBridge();
    ~ClaudeCodeBridge() override;

    // Disable copy, enable move
    ClaudeCodeBridge(const ClaudeCodeBridge&) = delete;
    ClaudeCodeBridge& operator=(const ClaudeCodeBridge&) = delete;
    ClaudeCodeBridge(ClaudeCodeBridge&&) noexcept = default;
    ClaudeCodeBridge& operator=(ClaudeCodeBridge&&) noexcept = default;

    // IAgentBridge implementation
    bool launch(const std::string& agent_id) override;
    bool sendTask(const std::string& task_description,
                const nlohmann::json& context) override;
    nlohmann::json waitForResult(int timeout_ms) override;
    bool terminate() override;
    bool isRunning() const override;

    std::string getAgentName() const override { return "Claude Code"; }
    std::string getAgentVersion() const override { return "VSCode Extension"; }

private:
    std::string vscode_path_;
    std::string workspace_path_;
    bool running_;
    pid_t agent_pid_;
};

} // namespace roboclaw::agent
