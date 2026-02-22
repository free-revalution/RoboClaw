#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace roboclaw::agent {

class IAgentBridge {
public:
    virtual ~IAgentBridge() = default;

    // Launch agent
    virtual bool launch(const std::string& agent_id) = 0;

    // Send task
    virtual bool sendTask(const std::string& task_description,
                        const nlohmann::json& context) = 0;

    // Wait for result
    virtual nlohmann::json waitForResult(int timeout_ms) = 0;

    // Terminate agent
    virtual bool terminate() = 0;

    // Check running status
    virtual bool isRunning() const = 0;

    // Get agent info
    virtual std::string getAgentName() const = 0;
    virtual std::string getAgentVersion() const = 0;
};

} // namespace roboclaw::agent
