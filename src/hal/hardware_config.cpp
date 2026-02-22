#include "hardware_config.h"
#include "hal_exception.h"
#include <fstream>
#include <iostream>

namespace roboclaw::hal {

bool HardwareConfig::loadFromFile(const std::string& path) {
    std::ifstream file(path);

    if (!file.is_open()) {
        return false;
    }

    try {
        file >> config_;
        loaded_ = true;
        return true;
    } catch (const nlohmann::json::parse_error& e) {
        throw HardwareException("HardwareConfig", "Failed to parse JSON: " + std::string(e.what()));
    } catch (const std::exception& e) {
        throw HardwareException("HardwareConfig", "Failed to load config: " + std::string(e.what()));
    }
}

nlohmann::json HardwareConfig::getMotorConfig(const std::string& name) const {
    if (!loaded_) {
        return nlohmann::json::object();
    }

    // Check if motors section exists
    if (!config_.contains("motors")) {
        return nlohmann::json::object();
    }

    const auto& motors = config_["motors"];

    // Check if specific motor exists
    if (!motors.contains(name)) {
        return nlohmann::json::object();
    }

    return motors[name];
}

nlohmann::json HardwareConfig::getSensorConfig(const std::string& name) const {
    if (!loaded_) {
        return nlohmann::json::object();
    }

    // Check if sensors section exists
    if (!config_.contains("sensors")) {
        return nlohmann::json::object();
    }

    const auto& sensors = config_["sensors"];

    // Check if specific sensor exists
    if (!sensors.contains(name)) {
        return nlohmann::json::object();
    }

    return sensors[name];
}

std::vector<std::string> HardwareConfig::getMotorNames() const {
    std::vector<std::string> names;

    if (!loaded_ || !config_.contains("motors")) {
        return names;
    }

    const auto& motors = config_["motors"];

    for (auto it = motors.begin(); it != motors.end(); ++it) {
        names.push_back(it.key());
    }

    return names;
}

std::vector<std::string> HardwareConfig::getSensorNames() const {
    std::vector<std::string> names;

    if (!loaded_ || !config_.contains("sensors")) {
        return names;
    }

    const auto& sensors = config_["sensors"];

    for (auto it = sensors.begin(); it != sensors.end(); ++it) {
        names.push_back(it.key());
    }

    return names;
}

bool HardwareConfig::hasMotor(const std::string& name) const {
    if (!loaded_ || !config_.contains("motors")) {
        return false;
    }

    return config_["motors"].contains(name);
}

bool HardwareConfig::hasSensor(const std::string& name) const {
    if (!loaded_ || !config_.contains("sensors")) {
        return false;
    }

    return config_["sensors"].contains(name);
}

} // namespace roboclaw::hal
