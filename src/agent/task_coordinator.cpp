#include "task_coordinator.h"
#include <fstream>
#include <algorithm>

namespace roboclaw::agent {

TaskCoordinator::TaskCoordinator() {
    loadAgentCapabilities();
}

void TaskCoordinator::loadAgentCapabilities() {
    // Load predefined agent capability matrix
    AgentCapabilities claude_code;
    claude_code.agent_id = "claude-code";
    claude_code.avg_response_time = 2.0;
    claude_code.reliability = 0.95;

    AgentCapability cpp_coding;
    cpp_coding.id = "cpp-coding";
    cpp_coding.name = "C++ Programming";
    cpp_coding.category = "coding";
    cpp_coding.tags = {"cpp", "embedded", "stm32", "arduino"};
    cpp_coding.proficiency = 95;

    claude_code.capabilities.push_back(cpp_coding);
    agent_capabilities_["claude-code"] = claude_code;

    // TODO: Add other agent capabilities
}

TaskAnalysis TaskCoordinator::analyzeTask(const nlohmann::json& task_description) {
    TaskAnalysis analysis;

    std::string desc = task_description.value("description", "");

    // Detect programming language
    if (desc.find("C++") != std::string::npos ||
        desc.find("cpp") != std::string::npos) {
        analysis.language = "cpp";
    } else if (desc.find("Python") != std::string::npos) {
        analysis.language = "python";
    } else {
        analysis.language = "general";
    }

    // Detect task type
    if (desc.find("写") != std::string::npos ||
        desc.find("实现") != std::string::npos ||
        desc.find("Write") != std::string::npos ||
        desc.find("Implement") != std::string::npos) {
        analysis.category = "coding";
    } else if (desc.find("分析") != std::string::npos ||
               desc.find("debug") != std::string::npos) {
        analysis.category = "analysis";
    } else {
        analysis.category = "general";
    }

    // Detect complexity
    if (desc.find("模块") != std::string::npos ||
        desc.find("系统") != std::string::npos) {
        analysis.complexity = "high";
    } else {
        analysis.complexity = "medium";
    }

    analysis.requires_domain_expertise = (analysis.complexity == "high");

    return analysis;
}

std::string TaskCoordinator::selectBestAgent(const TaskAnalysis& analysis) {
    std::string best_agent;
    double best_score = 0.0;

    for (const auto& [agent_id, capabilities] : agent_capabilities_) {
        double score = calculateFitnessScore(analysis, capabilities);
        if (score > best_score) {
            best_score = score;
            best_agent = agent_id;
        }
    }

    return best_score > 70.0 ? best_agent : "";
}

double TaskCoordinator::calculateFitnessScore(
    const TaskAnalysis& task,
    const AgentCapabilities& agent) {

    double score = 0.0;

    // Check capability match (40%)
    for (const auto& cap : agent.capabilities) {
        if (cap.category == task.category) {
            score += 40.0;

            // Check language tag match
            for (const auto& tag : cap.tags) {
                if (task.language.find(tag) != std::string::npos) {
                    score += 10.0;
                    break;
                }
            }

            // Consider proficiency
            score += (cap.proficiency / 100.0) * 20.0;
            break;
        }
    }

    // Consider reliability (20%)
    score += agent.reliability * 20.0;

    // Consider response speed (10%)
    const double max_response_time = 5.0;
    double response_score = 1.0 - (agent.avg_response_time / max_response_time);
    score += response_score * 10.0;

    return score;
}

bool TaskCoordinator::shouldDelegate(const TaskAnalysis& analysis,
                                   const std::string& agent_id) {
    auto it = agent_capabilities_.find(agent_id);
    if (it == agent_capabilities_.end()) {
        return false;
    }

    double score = calculateFitnessScore(analysis, it->second);
    return score >= 70.0 && it->second.reliability >= 0.8;
}

bool TaskCoordinator::delegateToAgent(const std::string& agent_id,
                                     const std::string& task_description,
                                     const nlohmann::json& context) {
    // TODO: Implement actual agent delegation logic
    // This requires integration with Agent Bridge
    return true;
}

} // namespace roboclaw::agent
