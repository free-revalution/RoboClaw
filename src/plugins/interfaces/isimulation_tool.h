// src/plugins/interfaces/isimulation_tool.h
#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "../plugin.h"

namespace roboclaw::plugins {

/**
 * @brief Test scenario for simulation
 */
struct TestScenario {
    std::string name;             // Scenario name
    nlohmann::json config;        // Scenario configuration
    double duration;              // Test duration in seconds
    std::vector<std::string> metrics_to_collect;  // Metrics to record
};

/**
 * @brief Result from a simulation test
 */
struct SimulationResult {
    bool success;                 // Whether test passed
    double duration;              // Actual test duration
    nlohmann::json metrics;       // Collected metrics
    std::string error_message;    // Error message if failed
    std::vector<std::string> log_entries;  // Test log entries
};

/**
 * @brief Interface for simulation tool plugins
 *
 * Simulation tool plugins provide integration with robotics simulators
 * like Gazebo, Webots, and custom simulation environments.
 */
class ISimulationTool : public IPlugin {
public:
    virtual ~ISimulationTool() = default;

    /**
     * @brief Load a robot model into the simulator
     * @param model_path Path to model file (URDF/SDF/etc.)
     * @return true if model loaded successfully
     */
    virtual bool loadModel(const std::string& model_path) = 0;

    /**
     * @brief Unload the current model
     */
    virtual void unloadModel() = 0;

    /**
     * @brief Run a test scenario
     * @param scenario Test scenario to execute
     * @return Simulation result with metrics
     */
    virtual SimulationResult runTest(const TestScenario& scenario) = 0;

    /**
     * @brief Extract metrics from the simulation
     * @return JSON object containing all available metrics
     */
    virtual nlohmann::json extractMetrics() = 0;

    /**
     * @brief Get a specific metric value
     * @param metric_name Name of the metric
     * @return Metric value, or null if not found
     */
    virtual nlohmann::json getMetric(const std::string& metric_name) = 0;

    /**
     * @brief Synchronize parameters from simulation to hardware
     * @param params Parameters to sync
     * @return true if sync successful
     */
    virtual bool syncParametersToHardware(const nlohmann::json& params) = 0;

    /**
     * @brief Start the simulation
     * @return true if simulation started successfully
     */
    virtual bool startSimulation() = 0;

    /**
     * @brief Stop the simulation
     */
    virtual void stopSimulation() = 0;

    /**
     * @brief Pause the simulation
     */
    virtual void pauseSimulation() = 0;

    /**
     * @brief Resume the simulation
     */
    virtual void resumeSimulation() = 0;

    /**
     * @brief Reset the simulation to initial state
     */
    virtual void resetSimulation() = 0;

    /**
     * @brief Check if simulation is running
     * @return true if simulation is active
     */
    virtual bool isRunning() const = 0;

    /**
     * @brief Check if a model is currently loaded
     * @return true if model is loaded
     */
    virtual bool isModelLoaded() const = 0;

    /**
     * @brief Set simulation time step
     * @param dt Time step in seconds
     */
    virtual void setTimeStep(double dt) = 0;

    /**
     * @brief Get current simulation time
     * @return Current simulation time in seconds
     */
    virtual double getSimulationTime() const = 0;
};

} // namespace roboclaw::plugins
