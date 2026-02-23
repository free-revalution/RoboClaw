// src/simulation/sim2real_transfer.cpp
#include "sim2real_transfer.h"
#include <algorithm>
#include <cmath>

namespace roboclaw::simulation {

Sim2RealTransfer::Sim2RealTransfer() {
}

//=============================================================================
// Parameter Mapping
//=============================================================================

nlohmann::json Sim2RealTransfer::mapSimulationToHardware(const nlohmann::json& sim_params,
                                                          const CalibrationMatrix& calibration) {
    nlohmann::json hw_params;

    for (auto& [key, value] : sim_params.items()) {
        if (value.is_number()) {
            double sim_value = value.get<double>();
            double hw_value = applyCalibration(sim_value, key, calibration);
            hw_params[key] = hw_value;
        } else if (value.is_object()) {
            // Handle nested objects (like PID parameter sets)
            nlohmann::json nested_hw;
            for (auto& [sub_key, sub_value] : value.items()) {
                if (sub_value.is_number()) {
                    double sim_value = sub_value.get<double>();
                    double hw_value = applyCalibration(sim_value, sub_key, calibration);
                    nested_hw[sub_key] = hw_value;
                } else {
                    nested_hw[sub_key] = sub_value;
                }
            }
            hw_params[key] = nested_hw;
        } else {
            hw_params[key] = value;
        }
    }

    return hw_params;
}

double Sim2RealTransfer::applyCalibration(double value, const std::string& param_name,
                                           const CalibrationMatrix& calibration) {
    double calibrated = value;

    // Apply general calibration
    if (param_name.find("position") != std::string::npos) {
        calibrated *= calibration.position_scale;
    } else if (param_name.find("velocity") != std::string::npos || param_name == "kp") {
        calibrated *= calibration.velocity_scale;
    } else if (param_name.find("effort") != std::string::npos || param_name == "kd") {
        calibrated *= calibration.effort_scale;
    }

    // Apply sensor offset
    calibrated += calibration.sensor_offset;

    // Apply per-axis calibration if available
    if (calibration.per_axis_calibration.contains(param_name)) {
        auto axis_cal = calibration.per_axis_calibration[param_name];
        if (axis_cal.contains("scale")) {
            calibrated *= axis_cal["scale"].get<double>();
        }
        if (axis_cal.contains("offset")) {
            calibrated += axis_cal["offset"].get<double>();
        }
    }

    return calibrated;
}

//=============================================================================
// Safety Verification
//=============================================================================

SafetyCheckResult Sim2RealTransfer::verifySafeDeployment(const nlohmann::json& hardware_params) {
    SafetyCheckResult result;
    result.passed = true;

    // Check PID parameters
    if (hardware_params.contains("speed_pid") || hardware_params.contains("position_pid")) {
        auto pid_params = hardware_params.contains("speed_pid")
            ? hardware_params["speed_pid"]
            : hardware_params["position_pid"];

        if (!arePIDParametersSafe(pid_params)) {
            result.passed = false;
            result.errors.push_back("PID parameters exceed safe limits");

            // Adjust parameters to safe range
            nlohmann::json adjusted;
            adjusted["kp"] = std::min(pid_params.value("kp", 1.0), 10.0);
            adjusted["ki"] = std::min(pid_params.value("ki", 0.0), 5.0);
            adjusted["kd"] = std::min(pid_params.value("kd", 0.0), 2.0);
            result.adjusted_params = adjusted;
        }
    }

    // Check velocity limits
    if (hardware_params.contains("max_velocity")) {
        if (!areVelocityLimitsSafe(hardware_params)) {
            result.passed = false;
            result.errors.push_back("Velocity limits exceed safe range");
            result.adjusted_params["max_velocity"] = 2.0;  // Safe default
        }
    }

    // Generate warnings for borderline values
    if (hardware_params.contains("speed_pid")) {
        auto kp = hardware_params["speed_pid"].value("kp", 0.0);
        if (kp > 8.0) {
            result.warnings.push_back("High Kp value may cause oscillation");
        }
    }

    return result;
}

bool Sim2RealTransfer::arePIDParametersSafe(const nlohmann::json& params) {
    double kp = params.value("kp", 0.0);
    double ki = params.value("ki", 0.0);
    double kd = params.value("kd", 0.0);

    // Safety limits
    const double MAX_KP = 20.0;
    const double MAX_KI = 10.0;
    const double MAX_KD = 5.0;

    if (kp > MAX_KP || ki > MAX_KI || kd > MAX_KD) {
        return false;
    }

    // Check for unstable combinations
    if (kp > 10.0 && ki > 5.0) {
        return false;  // High kp + ki can cause instability
    }

    return true;
}

bool Sim2RealTransfer::areVelocityLimitsSafe(const nlohmann::json& limits) {
    double max_vel = limits.value("max_velocity", 0.0);
    return max_vel <= 5.0;  // 5 m/s or rad/s max
}

std::string Sim2RealTransfer::generateSafetyWarning(const std::string& param_name,
                                                    double value,
                                                    double max_safe) {
    std::stringstream ss;
    ss << param_name << " (" << value << ") exceeds safe limit (" << max_safe << ")";
    return ss.str();
}

//=============================================================================
// Progressive Deployment
//=============================================================================

bool Sim2RealTransfer::progressiveDeployment(const nlohmann::json& params,
                                              const std::vector<DeploymentStage>& stages,
                                              std::function<bool(int, const std::string&)> progress_callback) {
    for (size_t i = 0; i < stages.size(); ++i) {
        const auto& stage = stages[i];

        // TODO: Implement actual deployment to hardware
        // This would involve:
        // - Sending parameters to hardware with power limits
        // - Monitoring for errors
        // - Collecting telemetry
        // - Verifying expected behavior

        std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Simulate operation

        // Call progress callback
        if (progress_callback) {
            std::string message = "Stage " + std::to_string(i + 1) + "/" + std::to_string(stages.size()) +
                                 ": " + stage.description;
            if (!progress_callback(static_cast<int>(i), message)) {
                // Callback requested abort
                return false;
            }
        }
    }

    return true;
}

//=============================================================================
// Default Configuration
//=============================================================================

nlohmann::json Sim2RealTransfer::getDefaultSafetyLimits() const {
    return {
        {"max_kp", 20.0},
        {"max_ki", 10.0},
        {"max_kd", 5.0},
        {"max_velocity", 5.0},
        {"max_acceleration", 10.0},
        {"max_current", 5.0}
    };
}

std::vector<DeploymentStage> Sim2RealTransfer::getDefaultDeploymentStages() const {
    return {
        {30, 0.5, 1.0, 2.0, "Low power test - verify basic functionality"},
        {60, 1.0, 2.0, 3.0, "Medium power test - check performance"},
        {100, 2.0, 5.0, 5.0, "Full power test - final verification"}
    };
}

} // namespace roboclaw::simulation
