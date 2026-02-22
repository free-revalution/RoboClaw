#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <map>

namespace roboclaw::agent {

// Agent capability definition
struct AgentCapability {
    std::string id;
    std::string name;
    std::string category;    // coding, debugging, testing, analysis
    std::vector<std::string> tags;  // cpp, python, web, embedded
    int proficiency;        // 0-100
};

// Agent capability configuration
struct AgentCapabilities {
    std::string agent_id;
    std::vector<AgentCapability> capabilities;
    double avg_response_time;
    double reliability;
};

// Task analysis result
struct TaskAnalysis {
    std::string category;     // coding, debugging, testing, analysis
    std::string language;     // cpp, python, javascript, etc.
    std::string complexity;   // low, medium, high
    bool requires_domain_expertise;
};

class TaskCoordinator {
public:
    TaskCoordinator();
    ~TaskCoordinator() = default;

    // Disable copy, enable move
    TaskCoordinator(const TaskCoordinator&) = delete;
    TaskCoordinator& operator=(const TaskCoordinator&) = delete;
    TaskCoordinator(TaskCoordinator&&) noexcept = default;
    TaskCoordinator& operator=(TaskCoordinator&&) noexcept = default;

    // Analyze task
    TaskAnalysis analyzeTask(const nlohmann::json& task_description);

    // Select best agent
    std::string selectBestAgent(const TaskAnalysis& analysis);

    // Delegate task to agent
    bool delegateToAgent(const std::string& agent_id,
                         const std::string& task_description,
                         const nlohmann::json& context);

    // Check if should delegate
    bool shouldDelegate(const TaskAnalysis& analysis,
                      const std::string& agent_id);

private:
    // Load agent capabilities
    void loadAgentCapabilities();

    // Calculate fitness score
    double calculateFitnessScore(const TaskAnalysis& task,
                                  const AgentCapabilities& agent);

    std::map<std::string, AgentCapabilities> agent_capabilities_;
};

} // namespace roboclaw::agent
