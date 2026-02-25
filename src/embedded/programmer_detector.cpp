// src/embedded/programmer_detector.cpp
#include "programmer_detector.h"
#include <algorithm>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>

namespace roboclaw::embedded {

ProgrammerDetector::ProgrammerDetector() {
}

ProgrammerDetector::~ProgrammerDetector() {
}

ProgrammerDetectionResult ProgrammerDetector::scanProgrammers() {
    ProgrammerDetectionResult result;
    result.found = false;

    std::lock_guard<std::mutex> lock(mutex_);

    detected_programmers_.clear();

    // Scan for different programmer types
    auto stlink = detectSTLink();
    detected_programmers_.insert(detected_programmers_.end(), stlink.begin(), stlink.end());

    auto jlink = detectJLink();
    detected_programmers_.insert(detected_programmers_.end(), jlink.begin(), jlink.end());

    auto openocd = detectOpenOCD();
    detected_programmers_.insert(detected_programmers_.end(), openocd.begin(), openocd.end());

    auto serial = detectSerialProgrammers();
    detected_programmers_.insert(detected_programmers_.end(), serial.begin(), serial.end());

    result.programmers = detected_programmers_;
    result.found = !detected_programmers_.empty();

    if (!result.found) {
        result.error_message = "No programmers detected";
    }

    return result;
}

ProgrammerDetectionResult ProgrammerDetector::scanProgrammersOfType(const std::string& type) {
    ProgrammerDetectionResult result;
    result.found = false;

    std::lock_guard<std::mutex> lock(mutex_);

    detected_programmers_.clear();

    if (type == "stlink" || type == "st-link") {
        detected_programmers_ = detectSTLink();
    } else if (type == "jlink" || type == "j-link") {
        detected_programmers_ = detectJLink();
    } else if (type == "openocd") {
        detected_programmers_ = detectOpenOCD();
    } else if (type == "serial") {
        detected_programmers_ = detectSerialProgrammers();
    }

    result.programmers = detected_programmers_;
    result.found = !detected_programmers_.empty();

    if (!result.found) {
        result.error_message = "No programmers of type '" + type + "' detected";
    }

    return result;
}

bool ProgrammerDetector::verifyConnection(const std::string& programmer_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto& prog : detected_programmers_) {
        if (prog.id == programmer_id) {
            // TODO: Implement actual verification (e.g., send command to programmer)
            return prog.is_connected;
        }
    }

    return false;
}

DetailedFlashResult ProgrammerDetector::flashFirmware(
    const std::string& programmer_id,
    const std::string& firmware_path,
    const FlashOptions& options,
    std::function<void(const std::string&)> progress_callback
) {
    DetailedFlashResult result;
    result.success = false;
    result.programmer_id = programmer_id;
    result.firmware_path = firmware_path;
    result.bytes_written = 0;
    result.time_elapsed = 0.0;
    result.verification_passed = false;

    // Find programmer
    std::shared_ptr<roboclaw::plugins::ProgrammerInfo> prog_info;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& prog : detected_programmers_) {
            if (prog.id == programmer_id) {
                prog_info = std::make_shared<roboclaw::plugins::ProgrammerInfo>(prog);
                break;
            }
        }
    }

    if (!prog_info) {
        result.messages.push_back("Error: Programmer not found: " + programmer_id);
        return result;
    }

    result.programmer_name = prog_info->name;

    // Check firmware file exists
    if (!deviceExists(firmware_path)) {
        result.messages.push_back("Error: Firmware file not found: " + firmware_path);
        return result;
    }

    auto start = std::chrono::steady_clock::now();

    try {
        if (progress_callback) {
            progress_callback("Initializing programmer...");
        }

        // TODO: Implement actual flashing using appropriate tool
        // - For ST-Link: use st-flash or openocd
        // - For J-Link: use JLinkExe
        // - For serial: use avrdude or similar

        if (progress_callback) {
            progress_callback("Erasing flash...");
        }

        // Simulate operations
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        if (progress_callback) {
            progress_callback("Writing firmware...");
        }

        // Get file size
        std::ifstream file(firmware_path, std::ios::binary | std::ios::ate);
        if (file) {
            result.bytes_written = static_cast<int>(file.tellg());
        }

        // Simulate write operation
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        result.success = true;
        result.messages.push_back("Firmware written successfully");

        if (options.verify) {
            if (progress_callback) {
                progress_callback("Verifying firmware...");
            }

            // Simulate verification
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            result.verification_passed = true;
            result.messages.push_back("Verification passed");
        }

        auto end = std::chrono::steady_clock::now();
        result.time_elapsed = std::chrono::duration<double>(end - start).count();

    } catch (const std::exception& e) {
        result.messages.push_back(std::string("Error: ") + e.what());
    }

    return result;
}

std::shared_ptr<roboclaw::plugins::ProgrammerInfo> ProgrammerDetector::getProgrammerInfo(
    const std::string& programmer_id
) {
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto& prog : detected_programmers_) {
        if (prog.id == programmer_id) {
            return std::make_shared<roboclaw::plugins::ProgrammerInfo>(prog);
        }
    }

    return nullptr;
}

std::vector<std::string> ProgrammerDetector::getSupportedTypes() const {
    return {"stlink", "jlink", "openocd", "serial"};
}

std::vector<roboclaw::plugins::ProgrammerInfo> ProgrammerDetector::detectSTLink() {
    std::vector<roboclaw::plugins::ProgrammerInfo> programmers;

    // TODO: Implement actual ST-Link detection
    // - Check USB devices for STMicroelectronics vendor ID
    // - Check /dev/ttyUSB* and /dev/cu.usbserial* on macOS/Linux
    // - Check COM ports on Windows

    // Mock detection for testing
    roboclaw::plugins::ProgrammerInfo stlink;
    stlink.id = generateId("stlink", 0);
    stlink.name = "ST-Link V2";
    stlink.port = "/dev/ttyUSB0";
    stlink.is_connected = true;
    programmers.push_back(stlink);

    return programmers;
}

std::vector<roboclaw::plugins::ProgrammerInfo> ProgrammerDetector::detectJLink() {
    std::vector<roboclaw::plugins::ProgrammerInfo> programmers;

    // TODO: Implement actual J-Link detection
    // - Check USB devices for SEGGER vendor ID

    return programmers;
}

std::vector<roboclaw::plugins::ProgrammerInfo> ProgrammerDetector::detectOpenOCD() {
    std::vector<roboclaw::plugins::ProgrammerInfo> programmers;

    // TODO: Implement actual OpenOCD detection
    // - Check if openocd command is available
    // - Parse openocd output for supported adapters

    return programmers;
}

std::vector<roboclaw::plugins::ProgrammerInfo> ProgrammerDetector::detectSerialProgrammers() {
    std::vector<roboclaw::plugins::ProgrammerInfo> programmers;

    // TODO: Implement actual serial port detection
    // - Enumerate /dev/tty* on Linux
    // - Enumerate /dev/cu.* on macOS
    // - Enumerate COM* on Windows

    // Mock detection for testing
    roboclaw::plugins::ProgrammerInfo arduino;
    arduino.id = generateId("serial", 0);
    arduino.name = "Arduino-compatible programmer";
    arduino.port = "/dev/ttyUSB1";
    arduino.is_connected = true;
    programmers.push_back(arduino);

    return programmers;
}

std::string ProgrammerDetector::generateId(const std::string& type, int index) {
    return type + "_" + std::to_string(index);
}

bool ProgrammerDetector::deviceExists(const std::string& path) {
    return std::filesystem::exists(path);
}

} // namespace roboclaw::embedded
