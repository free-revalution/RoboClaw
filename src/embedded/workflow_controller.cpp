// src/embedded/workflow_controller.cpp
#include "workflow_controller.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace roboclaw::embedded {

WorkflowController::WorkflowController() {
}

WorkflowController::~WorkflowController() {
    // Platform is managed as shared_ptr, no need to delete
}

void WorkflowController::setPlatform(std::shared_ptr<roboclaw::plugins::IEmbeddedPlatform> platform) {
    platform_ = platform;
}

bool WorkflowController::configureCubeMX(const roboclaw::plugins::MCUModel& mcu,
                                        const roboclaw::plugins::PeripheralConfig& peripherals) {
    if (!platform_) {
        return false;
    }

    roboclaw::plugins::ProjectConfig config = getDefaultProjectConfig();
    config.mcu = mcu;
    config.peripherals = peripherals;

    return platform_->configureProject(config);
}

std::vector<std::string> WorkflowController::generateDriverCode(const roboclaw::plugins::DriverSpec& spec) {
    std::vector<std::string> generated_files;

    if (!platform_) {
        return generated_files;
    }

    // TODO: Generate actual code files
    // For now, return mock file paths
    if (platform_->generateCode(spec)) {
        generated_files.push_back("drivers/" + spec.driver_type + ".h");
        generated_files.push_back("drivers/" + spec.driver_type + ".cpp");
    }

    return generated_files;
}

nlohmann::json WorkflowController::optimizeParameters(const std::string& method,
                                                     const TestConfig& test,
                                                     const OptimizationConstraints& constraints) {
    if (!platform_) {
        return nlohmann::json{};
    }

    // Prepare test data (in real implementation, this would run actual tests)
    std::vector<roboclaw::plugins::TestResult> test_data;

    // Generate mock test results
    for (int i = 0; i < 10; ++i) {
        roboclaw::plugins::TestResult result;
        result.error_metric = 1.0 - (i * 0.1);  // Improving values
        result.params = {{"kp", 1.0 + i * 0.2}, {"ki", i * 0.1}, {"kd", 0.1}};
        result.timestamp = 0;
        test_data.push_back(result);
    }

    return platform_->optimizeParameters(method, nlohmann::json{}, test_data);
}

FlashResult WorkflowController::flashToFirmware(const std::string& firmware_path,
                                                const FlashOptions& options) {
    FlashResult result;
    result.success = false;
    result.bytes_written = 0;
    result.time_elapsed = 0.0;

    if (!platform_) {
        result.message = "No platform configured";
        return result;
    }

    // Detect available programmers
    auto programmers = platform_->detectProgrammers();
    if (programmers.empty()) {
        result.message = "No programmers detected";
        return result;
    }

    // Use first available programmer
    std::string programmer_id = programmers[0].id;

    auto start = std::chrono::steady_clock::now();

    // Flash firmware
    if (platform_->flashFirmware(firmware_path, programmer_id)) {
        result.success = true;
        result.message = "Firmware flashed successfully";

        // Verify if requested
        if (options.verify && platform_->verifyFirmware(firmware_path)) {
            result.message += " (verified)";
        }

        auto end = std::chrono::steady_clock::now();
        result.time_elapsed = std::chrono::duration<double>(end - start).count();

        // Get file size for bytes written
        // TODO: Implement actual file size reading
        result.bytes_written = 0;
    } else {
        result.message = "Firmware flashing failed";
    }

    return result;
}

bool WorkflowController::runFullWorkflow(const WorkflowSpec& spec) {
    if (!platform_) {
        return false;
    }

    // Step 1: Configure project
    if (spec.configure_project) {
        auto default_config = getDefaultProjectConfig();
        if (!platform_->configureProject(default_config)) {
            return false;
        }
    }

    // Step 2: Generate code
    if (spec.generate_code) {
        roboclaw::plugins::DriverSpec driver_spec{"motor_control", {}};
        if (!platform_->generateCode(driver_spec)) {
            return false;
        }
    }

    // Step 3: Optimize parameters
    if (spec.optimize_parameters) {
        TestConfig test_config{"step", 5.0, 1.0, 1.0, {}};
        OptimizationConstraints constraints;
        optimizeParameters(spec.optimization_method, test_config, constraints);
    }

    // Step 4: Build firmware
    if (spec.build_firmware) {
        if (!platform_->buildProject()) {
            return false;
        }
    }

    // Step 5: Flash firmware
    if (spec.flash_firmware) {
        FlashOptions options;
        auto result = flashToFirmware("build/firmware.elf", options);
        if (!result.success) {
            return false;
        }
    }

    return true;
}

std::vector<HardwareInfo> WorkflowController::scanConnectedHardware() {
    std::vector<HardwareInfo> hardware;

    if (!platform_) {
        return hardware;
    }

    auto programmers = platform_->scanConnectedHardware();

    // Convert ProgrammerInfo to HardwareInfo
    for (const auto& prog : programmers) {
        HardwareInfo info;
        info.id = prog.id;
        info.name = prog.name;
        info.type = "programmer";
        info.port = prog.port;
        info.is_connected = prog.is_connected;
        hardware.push_back(info);
    }

    return hardware;
}

std::vector<std::string> WorkflowController::getAvailableOptimizers() const {
    if (platform_) {
        return platform_->getOptimizationMethods();
    }
    return {"zigler_nichols", "genetic_algorithm", "bayesian_optimization"};
}

std::string WorkflowController::mcuModelToString(const roboclaw::plugins::MCUModel& mcu) const {
    switch (mcu) {
        case roboclaw::plugins::MCUModel::STM32_F0: return "STM32F0";
        case roboclaw::plugins::MCUModel::STM32_F1: return "STM32F1";
        case roboclaw::plugins::MCUModel::STM32_F4: return "STM32F4";
        case roboclaw::plugins::MCUModel::STM32_F7: return "STM32F7";
        case roboclaw::plugins::MCUModel::STM32_H7: return "STM32H7";
        case roboclaw::plugins::MCUModel::ESP32: return "ESP32";
        case roboclaw::plugins::MCUModel::ESP32_S3: return "ESP32-S3";
        case roboclaw::plugins::MCUModel::RP2040: return "RP2040";
        default: return "UNKNOWN";
    }
}

roboclaw::plugins::ProjectConfig WorkflowController::getDefaultProjectConfig() const {
    roboclaw::plugins::ProjectConfig config;
    config.mcu = roboclaw::plugins::MCUModel::STM32_F4;
    config.project_name = "roboclaw_project";
    config.peripherals = roboclaw::plugins::PeripheralConfig{};
    config.output_directory = "build";
    return config;
}

} // namespace roboclaw::embedded
