// src/embedded/workflow_controller.h
#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>

#include "plugins/interfaces/iembedded_platform.h"

namespace roboclaw::embedded {

/**
 * @brief Optimization constraints for parameter tuning
 */
struct OptimizationConstraints {
    double max_overshoot = 10.0;       // Maximum overshoot percentage
    double max_settling_time = 5.0;    // Maximum settling time in seconds
    double min_stability_margin = 0.5; // Minimum stability margin
    nlohmann::json param_ranges;       // Per-parameter ranges
};

/**
 * @brief Flash options for firmware programming
 */
struct FlashOptions {
    bool verify = true;                // Verify after flashing
    bool erase_chip = false;           // Full chip erase
    int programming_speed = 0;         // 0 = auto, 1-4 = speed level
};

/**
 * @brief Flash result structure
 */
struct FlashResult {
    bool success;
    std::string message;
    int bytes_written;
    double time_elapsed;
};

/**
 * @brief Test configuration for optimization
 */
struct TestConfig {
    std::string test_type;             // "step", "sine", "square_wave"
    double duration;                   // Test duration in seconds
    double amplitude;                  // Test amplitude
    double frequency;                  // Test frequency (for periodic signals)
    nlohmann::json additional_params;
};

/**
 * @brief Hardware information structure
 */
struct HardwareInfo {
    std::string id;
    std::string name;
    std::string type;                  // "programmer", "mcu"
    std::string port;
    bool is_connected;
    nlohmann::json capabilities;
};

/**
 * @brief Workflow specification for end-to-end automation
 */
struct WorkflowSpec {
    bool configure_project = true;
    bool generate_code = true;
    bool optimize_parameters = false;
    bool build_firmware = true;
    bool flash_firmware = false;
    std::string optimization_method = "zigler_nichols";
};

/**
 * @brief Embedded Development Workflow Controller
 *
 * The WorkflowController orchestrates the complete embedded development
 * process from project configuration to firmware flashing.
 *
 * Natural language dialogue examples:
 * - "Create STM32F407 project with UART1 and SPI1"
 * - "Write PID control driver for DC motor"
 * - "Tune speed loop PID parameters"
 * - "Flash firmware to MCU"
 */
class WorkflowController {
public:
    WorkflowController();
    ~WorkflowController();

    // Disable copy
    WorkflowController(const WorkflowController&) = delete;
    WorkflowController& operator=(const WorkflowController&) = delete;

    /**
     * @brief Set the embedded platform plugin to use
     * @param platform Shared pointer to platform plugin
     */
    void setPlatform(std::shared_ptr<roboclaw::plugins::IEmbeddedPlatform> platform);

    /**
     * @brief Configure CubeMX project
     * @param mcu MCU model
     * @param peripherals Peripheral configuration
     * @return true if successful
     */
    bool configureCubeMX(const roboclaw::plugins::MCUModel& mcu,
                        const roboclaw::plugins::PeripheralConfig& peripherals);

    /**
     * @brief Generate driver code
     * @param spec Driver specification
     * @return Generated file paths
     */
    std::vector<std::string> generateDriverCode(const roboclaw::plugins::DriverSpec& spec);

    /**
     * @brief Optimize control parameters
     * @param method Optimization method
     * @param test Test configuration
     * @param constraints Optimization constraints
     * @return Optimized parameters
     */
    nlohmann::json optimizeParameters(const std::string& method,
                                     const TestConfig& test,
                                     const OptimizationConstraints& constraints);

    /**
     * @brief Flash firmware to MCU
     * @param firmware_path Path to firmware file
     * @param options Flash options
     * @return Flash result
     */
    FlashResult flashToFirmware(const std::string& firmware_path,
                                const FlashOptions& options = FlashOptions{});

    /**
     * @brief Run complete workflow
     * @param spec Workflow specification
     * @return true if all steps successful
     */
    bool runFullWorkflow(const WorkflowSpec& spec);

    /**
     * @brief Scan for connected hardware
     * @return List of detected hardware
     */
    std::vector<HardwareInfo> scanConnectedHardware();

    /**
     * @brief Get available optimization methods
     * @return List of method names
     */
    std::vector<std::string> getAvailableOptimizers() const;

private:
    std::shared_ptr<roboclaw::plugins::IEmbeddedPlatform> platform_;

    /**
     * @brief Convert MCU model enum to string
     */
    std::string mcuModelToString(const roboclaw::plugins::MCUModel& mcu) const;

    /**
     * @brief Get default project config
     */
    roboclaw::plugins::ProjectConfig getDefaultProjectConfig() const;
};

} // namespace roboclaw::embedded
