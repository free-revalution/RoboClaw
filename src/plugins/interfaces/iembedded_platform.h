// src/plugins/interfaces/iembedded_platform.h
#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "../plugin.h"

namespace roboclaw::plugins {

/**
 * @brief MCU model enumeration
 */
enum class MCUModel {
    STM32_F0, STM32_F1, STM32_F2, STM32_F3, STM32_F4, STM32_F7, STM32_H7,
    ESP32, ESP32_C3, ESP32_S2, ESP32_S3,
    ARDUINO_AVR, ARDUINO_SAM, ARDUINO_SAMD,
    RP2040,
    UNKNOWN
};

/**
 * @brief Peripheral configuration structure
 */
struct PeripheralConfig {
    nlohmann::json peripherals;  // JSON describing enabled peripherals
};

/**
 * @brief Project configuration structure
 */
struct ProjectConfig {
    MCUModel mcu;
    std::string project_name;
    PeripheralConfig peripherals;
    std::string output_directory;
};

/**
 * @brief Driver specification for code generation
 */
struct DriverSpec {
    std::string driver_type;      // e.g., "motor_control", "sensor", "communication"
    nlohmann::json parameters;    // Driver-specific parameters
};

/**
 * @brief Test result for parameter optimization
 */
struct TestResult {
    double error_metric;          // Error value to minimize
    nlohmann::json params;        // Parameter values used
    int64_t timestamp;            // Test timestamp
};

/**
 * @brief Programmer information structure
 */
struct ProgrammerInfo {
    std::string id;               // Programmer ID
    std::string name;             // Programmer name
    std::string port;             // Port/device path
    bool is_connected;            // Connection status
};

/**
 * @brief Interface for embedded platform plugins
 *
 * Embedded platform plugins provide support for specific MCU platforms,
 * including project configuration, code generation, parameter optimization,
 * and firmware flashing capabilities.
 */
class IEmbeddedPlatform : public IPlugin {
public:
    virtual ~IEmbeddedPlatform() = default;

    // ========================================================================
    // Project Configuration
    // ========================================================================

    /**
     * @brief Configure the project with CubeMX or similar tool
     * @param mcu MCU model to configure
     * @param peripherals Peripheral configuration
     * @return true if configuration successful
     */
    virtual bool configureProject(const ProjectConfig& config) = 0;

    /**
     * @brief Generate driver code from specification
     * @param spec Driver specification
     * @return true if code generation successful
     */
    virtual bool generateCode(const DriverSpec& spec) = 0;

    /**
     * @brief Build the project (compile firmware)
     * @return true if build successful
     */
    virtual bool buildProject() = 0;

    // ========================================================================
    // Parameter Optimization
    // ========================================================================

    /**
     * @brief Optimize control parameters using specified method
     * @param method Optimization method (e.g., "zigler_nichols", "genetic", "bayesian")
     * @param current_params Current parameter values
     * @param test_data Test data for optimization
     * @return Optimized parameter values
     */
    virtual nlohmann::json optimizeParameters(
        const std::string& method,
        const nlohmann::json& current_params,
        const std::vector<TestResult>& test_data
    ) = 0;

    /**
     * @brief Get available optimization methods
     * @return List of optimization method names
     */
    virtual std::vector<std::string> getOptimizationMethods() const = 0;

    // ========================================================================
    // Flashing
    // ========================================================================

    /**
     * @brief Detect available programmers
     * @return List of detected programmers
     */
    virtual std::vector<ProgrammerInfo> detectProgrammers() = 0;

    /**
     * @brief Flash firmware to the MCU
     * @param firmware_path Path to firmware file
     * @param programmer_id ID of programmer to use
     * @return true if flashing successful
     */
    virtual bool flashFirmware(const std::string& firmware_path, const std::string& programmer_id) = 0;

    /**
     * @brief Verify flashed firmware
     * @param firmware_path Path to original firmware file
     * @return true if verification successful
     */
    virtual bool verifyFirmware(const std::string& firmware_path) = 0;

    // ========================================================================
    // Hardware Detection
    // ========================================================================

    /**
     * @brief Scan for connected hardware
     * @return List of detected programmers and MCUs
     */
    virtual std::vector<ProgrammerInfo> scanConnectedHardware() = 0;
};

} // namespace roboclaw::plugins
