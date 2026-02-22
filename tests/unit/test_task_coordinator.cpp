#include <gtest/gtest.h>
#include "agent/task_coordinator.h"

using namespace roboclaw::agent;

TEST(TaskCoordinator, CanAnalyzeTask) {
    TaskCoordinator coordinator;

    nlohmann::json task = {
        {"type", "coding"},
        {"language", "cpp"},
        {"description", "Write a serial communication module in cpp"}
    };

    auto analysis = coordinator.analyzeTask(task);

    EXPECT_EQ(analysis.category, "coding");
    EXPECT_EQ(analysis.language, "cpp");
}

TEST(TaskCoordinator, CanSelectAgent) {
    TaskCoordinator coordinator;

    TaskAnalysis analysis;
    analysis.category = "coding";
    analysis.language = "cpp";
    analysis.complexity = "high";

    std::string selected = coordinator.selectBestAgent(analysis);

    EXPECT_FALSE(selected.empty());
    EXPECT_EQ(selected, "claude-code");
}

TEST(TaskCoordinator, ShouldDelegateWhenScoreHigh) {
    TaskCoordinator coordinator;

    TaskAnalysis analysis;
    analysis.category = "coding";
    analysis.language = "cpp";
    analysis.complexity = "high";

    bool should_delegate = coordinator.shouldDelegate(analysis, "claude-code");

    EXPECT_TRUE(should_delegate);
}

TEST(TaskCoordinator, ShouldNotDelegateWhenAgentUnknown) {
    TaskCoordinator coordinator;

    TaskAnalysis analysis;
    analysis.category = "coding";
    analysis.language = "python";
    analysis.complexity = "high";

    bool should_delegate = coordinator.shouldDelegate(analysis, "unknown-agent");

    EXPECT_FALSE(should_delegate);
}
