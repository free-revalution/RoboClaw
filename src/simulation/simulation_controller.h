// src/simulation/simulation_controller.h
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

#include "plugins/interfaces/isimulation_tool.h"

namespace roboclaw::simulation {

/**
 * @brief Hardware configuration for URDF/SDF generation
 */
struct HardwareConfig {
    std::string robot_name;
    std::vector<std::string> drive_wheels;
    std::vector<std::string> caster_wheels;
    std::vector<std::string> sensors;
    nlohmann::json dimensions;        // Robot dimensions
    nlohmann::json sensor_mounts;     // Sensor mounting positions
};

/**
 * @brief Test sequence for automated testing
 */
struct TestSequence {
    std::string description;
    std::vector<nlohmann::json> actions;  // Sequence of actions
    double timeout;
};

/**
 * @brief Test report with comprehensive results
 */
struct TestReport {
    std::string test_name;
    bool passed;
    int passed_count;
    int failed_count;
    double total_duration;
    std::vector<std::string> failures;
    std::vector<nlohmann::json> metrics;
    std::string html_report_path;
};

/**
 * @brief Simulation Controller
 *
 * Manages robotics simulation using Gazebo/ROS 2.
 * Handles model generation, test execution, and metrics extraction.
 *
 * Natural language examples:
 * - "Generate Gazebo model from current hardware config"
 * - "Run navigation test scenario"
 * - "Extract optimized PID parameters from simulation"
 */
class SimulationController {
public:
    SimulationController();
    ~SimulationController();

    // Disable copy
    SimulationController(const SimulationController&) = delete;
    SimulationController& operator=(const SimulationController&) = delete;

    /**
     * @brief Set the simulation tool plugin
     * @param tool Shared pointer to simulation tool
     */
    void setSimulationTool(std::shared_ptr<roboclaw::plugins::ISimulationTool> tool);

    // ========================================================================
    // Model Management
    // ========================================================================

    /**
     * @brief Generate URDF model from hardware configuration
     * @param config Hardware configuration
     * @return Path to generated URDF file
     */
    std::string generateURDF(const HardwareConfig& config);

    /**
     * @brief Generate SDF model from hardware configuration
     * @param config Hardware configuration
     * @return Path to generated SDF file
     */
    std::string generateSDF(const HardwareConfig& config);

    /**
     * @brief Load simulation model
     * @param model_path Path to model file
     * @return true if loaded successfully
     */
    bool loadSimulation(const std::string& model_path);

    /**
     * @brief Unload current model
     */
    void unloadSimulation();

    // ========================================================================
    // Test Execution
    // ========================================================================

    /**
     * @brief Run a test scenario
     * @param scenario Test scenario to run
     * @return Test result
     */
    roboclaw::plugins::SimulationResult runTestScenario(const roboclaw::plugins::TestScenario& scenario);

    /**
     * @brief Generate comprehensive test report
     * @param results Vector of test results
     * @param output_path Path for report output
     * @return Test report
     */
    TestReport generateTestReport(const std::vector<roboclaw::plugins::SimulationResult>& results,
                                  const std::string& output_path = "");

    /**
     * @brief Run batch tests
     * @param scenarios List of scenario names
     * @return Vector of test results
     */
    std::vector<roboclaw::plugins::SimulationResult> runBatchTests(const std::vector<std::string>& scenarios);

    // ========================================================================
    // Parameter Management
    // ========================================================================

    /**
     * @brief Extract optimized parameters from simulation
     * @return JSON with parameter values
     */
    nlohmann::json extractOptimizedParameters();

    /**
     * @brief Sync parameters to hardware
     * @param params Parameters to sync
     * @return true if successful
     */
    bool syncToHardware(const nlohmann::json& params);

    /**
     * @brief Get all available metrics
     * @return JSON with all metrics
     */
    nlohmann::json getAllMetrics();

    /**
     * @brief Get specific metric
     * @param metric_name Metric name
     * @return Metric value
     */
    nlohmann::json getMetric(const std::string& metric_name);

    // ========================================================================
    // ROS 2 Integration
    // ========================================================================

    /**
     * @brief Launch ROS 2 bridge
     * @return true if bridge launched
     */
    bool launchROS2Bridge();

    /**
     * @brief Publish message to ROS topic
     * @param topic Topic name
     * @param msg Message to publish
     */
    void publishToTopic(const std::string& topic, const nlohmann::json& msg);

    /**
     * @brief Subscribe to ROS topic
     * @param topic Topic name
     * @param callback Callback for received messages
     * @return true if subscription successful
     */
    bool subscribeTopic(const std::string& topic,
                       std::function<void(const nlohmann::json&)> callback);

    /**
     * @brief Shutdown ROS 2 bridge
     */
    void shutdownROS2Bridge();

private:
    std::shared_ptr<roboclaw::plugins::ISimulationTool> sim_tool_;
    bool ros2_bridge_active_;

    /**
     * @brief Generate robot link XML for URDF
     */
    std::string generateLinkXML(const std::string& name, const nlohmann::json& config);

    /**
     * @brief Generate joint XML for URDF
     */
    std::string generateJointXML(const std::string& name, const std::string& type,
                                const std::string& parent, const std::string& child,
                                const nlohmann::json& config);
};

} // namespace roboclaw::simulation
