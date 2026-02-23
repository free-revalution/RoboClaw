// src/simulation/sim2real_transfer.h
#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace roboclaw::simulation {

/**
 * @brief Calibration matrix for sim-to-real parameter transfer
 */
struct CalibrationMatrix {
    double position_scale = 1.0;
    double velocity_scale = 1.0;
    double effort_scale = 1.0;
    double sensor_offset = 0.0;
    nlohmann::json per_axis_calibration;  // Axis-specific calibrations
};

/**
 * @brief Safety check result
 */
struct SafetyCheckResult {
    bool passed;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
    nlohmann::json adjusted_params;  // Parameters adjusted for safety
};

/**
 * @brief Deployment stage for progressive rollout
 */
struct DeploymentStage {
    int power_percentage;      // Power limit (0-100)
    double max_velocity;       // Velocity limit
    double max_acceleration;   // Acceleration limit
    double duration;           // Stage duration in seconds
    std::string description;
};

/**
 * @brief Sim-to-Real Transfer Handler
 *
 * Manages the transfer of optimized parameters from simulation
 * to real hardware with safety validation and progressive deployment.
 *
 * Natural language examples:
 * - "Transfer optimized PID parameters to real hardware"
 * - "Validate parameters before deployment"
 * - "Progressive deployment with safety limits"
 */
class Sim2RealTransfer {
public:
    Sim2RealTransfer();
    ~Sim2RealTransfer() = default;

    /**
     * @brief Map simulation parameters to hardware
     * @param sim_params Parameters from simulation
     * @param calibration Calibration matrix
     * @return Hardware-adjusted parameters
     */
    nlohmann::json mapSimulationToHardware(const nlohmann::json& sim_params,
                                          const CalibrationMatrix& calibration);

    /**
     * @brief Verify parameters are safe for hardware deployment
     * @param hardware_params Parameters to verify
     * @return Safety check result
     */
    SafetyCheckResult verifySafeDeployment(const nlohmann::json& hardware_params);

    /**
     * @brief Progressive deployment with staged rollout
     * @param params Parameters to deploy
     * @param stages Deployment stages
     * @param progress_callback Optional callback for progress updates
     * @return true if all stages completed successfully
     */
    bool progressiveDeployment(const nlohmann::json& params,
                              const std::vector<DeploymentStage>& stages,
                              std::function<bool(int stage, const std::string&)> progress_callback = nullptr);

    /**
     * @brief Get default safety limits
     * @return JSON with safety limits
     */
    nlohmann::json getDefaultSafetyLimits() const;

    /**
     * @brief Get default deployment stages
     * @return Vector of default deployment stages
     */
    std::vector<DeploymentStage> getDefaultDeploymentStages() const;

    /**
     * @brief Apply calibration to single parameter
     * @param value Parameter value from simulation
     * @param param_name Parameter name
     * @param calibration Calibration matrix
     * @return Calibrated value for hardware
     */
    double applyCalibration(double value, const std::string& param_name,
                           const CalibrationMatrix& calibration);

private:
    /**
     * @brief Check if PID parameters are within safe range
     */
    bool arePIDParametersSafe(const nlohmann::json& params);

    /**
     * @brief Check if velocity limits are safe
     */
    bool areVelocityLimitsSafe(const nlohmann::json& limits);

    /**
     * @brief Generate safety warning for parameter
     */
    std::string generateSafetyWarning(const std::string& param_name, double value,
                                     double max_safe);
};

} // namespace roboclaw::simulation
