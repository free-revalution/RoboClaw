// src/embedded/programmer_detector.h
#pragma once

#include <string>
#include <vector>
#include <functional>

#include "plugins/interfaces/iembedded_platform.h"
#include "workflow_controller.h"

namespace roboclaw::embedded {

/**
 * @brief Programmer detection result
 */
struct ProgrammerDetectionResult {
    bool found;
    std::vector<roboclaw::plugins::ProgrammerInfo> programmers;
    std::string error_message;
};

/**
 * @brief Flash operation result with detailed information
 */
struct DetailedFlashResult {
    bool success;
    std::string programmer_id;
    std::string programmer_name;
    std::string firmware_path;
    int bytes_written;
    double time_elapsed;
    bool verification_passed;
    std::vector<std::string> messages;  // Progress/error messages
};

/**
 * @brief Programmer Detector and Flash Manager
 *
 * Detects and manages hardware programmers for flashing firmware to MCUs.
 * Supports ST-Link, J-Link, OpenOCD, and other programmers.
 */
class ProgrammerDetector {
public:
    ProgrammerDetector();
    ~ProgrammerDetector();

    /**
     * @brief Scan for all connected programmers
     * @return Detection result with list of programmers
     */
    ProgrammerDetectionResult scanProgrammers();

    /**
     * @brief Detect programmers of a specific type
     * @param type Programmer type ("stlink", "jlink", "openocd", etc.)
     * @return Detection result with filtered list
     */
    ProgrammerDetectionResult scanProgrammersOfType(const std::string& type);

    /**
     * @brief Verify connection to a specific programmer
     * @param programmer_id Programmer ID to verify
     * @return true if programmer is accessible
     */
    bool verifyConnection(const std::string& programmer_id);

    /**
     * @brief Flash firmware with detailed progress reporting
     * @param programmer_id Target programmer ID
     * @param firmware_path Path to firmware file
     * @param options Flash options
     * @param progress_callback Optional callback for progress updates
     * @return Detailed flash result
     */
    DetailedFlashResult flashFirmware(
        const std::string& programmer_id,
        const std::string& firmware_path,
        const FlashOptions& options = FlashOptions{},
        std::function<void(const std::string&)> progress_callback = nullptr
    );

    /**
     * @brief Get detailed information about a programmer
     * @param programmer_id Programmer ID
     * @return Programmer information, or null if not found
     */
    std::shared_ptr<roboclaw::plugins::ProgrammerInfo> getProgrammerInfo(const std::string& programmer_id);

    /**
     * @brief Get list of supported programmer types
     * @return List of programmer type names
     */
    std::vector<std::string> getSupportedTypes() const;

private:
    std::vector<roboclaw::plugins::ProgrammerInfo> detected_programmers_;
    mutable std::mutex mutex_;

    /**
     * @brief Detect ST-Link programmers
     */
    std::vector<roboclaw::plugins::ProgrammerInfo> detectSTLink();

    /**
     * @brief Detect J-Link programmers
     */
    std::vector<roboclaw::plugins::ProgrammerInfo> detectJLink();

    /**
     * @brief Detect OpenOCD-compatible programmers
     */
    std::vector<roboclaw::plugins::ProgrammerInfo> detectOpenOCD();

    /**
     * @brief Detect serial port programmers (Arduino-style)
     */
    std::vector<roboclaw::plugins::ProgrammerInfo> detectSerialProgrammers();

    /**
     * @brief Generate unique programmer ID
     */
    std::string generateId(const std::string& type, int index);

    /**
     * @brief Check if a USB device exists at path
     */
    bool deviceExists(const std::string& path);
};

} // namespace roboclaw::embedded
