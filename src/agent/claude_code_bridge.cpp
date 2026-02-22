#include "agent/claude_code_bridge.h"
#include "utils/logger.h"
#include <sys/wait.h>
#include <signal.h>
#include <cstdlib>
#include <filesystem>

namespace roboclaw::agent {

ClaudeCodeBridge::ClaudeCodeBridge()
    : vscode_path_("/Applications/Visual Studio Code.app/Contents/Resources/app/bin/code")
    , workspace_path_(std::filesystem::current_path().string())
    , running_(false)
    , agent_pid_(-1)
{
}

ClaudeCodeBridge::~ClaudeCodeBridge() {
    if (running_) {
        terminate();
    }
}

bool ClaudeCodeBridge::launch(const std::string& agent_id) {
    if (running_) {
        LOG_WARNING("ClaudeCodeBridge already running");
        return false;
    }

    LOG_INFO("Launching VSCode with workspace: " + workspace_path_);

    pid_t pid = fork();
    if (pid == -1) {
        LOG_ERROR("Failed to fork process for VSCode");
        return false;
    }

    if (pid == 0) {
        // Child process: execute VSCode
        execlp(vscode_path_.c_str(), "code", workspace_path_.c_str(), nullptr);

        // If execlp returns, an error occurred
        exit(EXIT_FAILURE);
    }

    // Parent process: store child PID
    agent_pid_ = pid;
    running_ = true;

    // Give VSCode a moment to start
    usleep(500000); // 500ms

    LOG_INFO("VSCode launched with PID: " + std::to_string(agent_pid_));
    return true;
}

bool ClaudeCodeBridge::sendTask(const std::string& task_description,
                                const nlohmann::json& context) {
    if (!running_) {
        LOG_ERROR("Cannot send task: ClaudeCodeBridge not running");
        return false;
    }

    // TODO: Implement VSCode Extension IPC for task sending
    // This requires:
    // 1. Setting up a communication channel (e.g., Unix domain socket, named pipe)
    // 2. Serializing the task description and context
    // 3. Sending to the VSCode extension
    // 4. Waiting for acknowledgment

    LOG_INFO("sendTask called (IPC not yet implemented): " + task_description);
    return false;
}

nlohmann::json ClaudeCodeBridge::waitForResult(int timeout_ms) {
    if (!running_) {
        return {
            {"success", false},
            {"error", "ClaudeCodeBridge not running"}
        };
    }

    // TODO: Implement result waiting via VSCode Extension IPC
    // This requires:
    // 1. Waiting for response on the IPC channel
    // 2. Timeout handling
    // 3. Parsing the result JSON

    return {
        {"success", false},
        {"error", "IPC not yet implemented"}
    };
}

bool ClaudeCodeBridge::terminate() {
    if (!running_) {
        LOG_WARNING("ClaudeCodeBridge not running");
        return false;
    }

    LOG_INFO("Terminating VSCode process: " + std::to_string(agent_pid_));

    // Send SIGTERM to the VSCode process
    if (kill(agent_pid_, SIGTERM) == -1) {
        LOG_ERROR("Failed to send SIGTERM to VSCode process");
        return false;
    }

    // Wait for the process to terminate
    int status;
    pid_t result = waitpid(agent_pid_, &status, 0);
    if (result == -1) {
        LOG_ERROR("Failed to wait for VSCode process termination");
        return false;
    }

    running_ = false;
    agent_pid_ = -1;

    LOG_INFO("VSCode process terminated successfully");
    return true;
}

bool ClaudeCodeBridge::isRunning() const {
    if (!running_ || agent_pid_ == -1) {
        return false;
    }

    // Check if the process exists by sending signal 0
    if (kill(agent_pid_, 0) == -1) {
        if (errno == ESRCH) {
            // Process does not exist
            return false;
        }
    }

    return true;
}

} // namespace roboclaw::agent
