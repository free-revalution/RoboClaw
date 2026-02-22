#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <unordered_map>

namespace roboclaw::hal {

/**
 * @brief Hardware configuration manager
 *
 * Loads and manages hardware configuration from JSON files.
 * Supports motor controllers and sensors configuration.
 */
class HardwareConfig {
public:
    HardwareConfig() = default;
    ~HardwareConfig() = default;

    /**
     * @brief Load configuration from JSON file
     * @param path Path to configuration file
     * @return true if loading succeeded, false otherwise
     */
    bool loadFromFile(const std::string& path);

    /**
     * @brief Get motor configuration by name
     * @param name Motor name
     * @return JSON configuration object (empty if not found)
     */
    nlohmann::json getMotorConfig(const std::string& name) const;

    /**
     * @brief Get sensor configuration by name
     * @param name Sensor name
     * @return JSON configuration object (empty if not found)
     */
    nlohmann::json getSensorConfig(const std::string& name) const;

    /**
     * @brief Get all configured motor names
     * @return Vector of motor names
     */
    std::vector<std::string> getMotorNames() const;

    /**
     * @brief Get all configured sensor names
     * @return Vector of sensor names
     */
    std::vector<std::string> getSensorNames() const;

    /**
     * @brief Check if motor configuration exists
     * @param name Motor name
     * @return true if motor exists in configuration
     */
    bool hasMotor(const std::string& name) const;

    /**
     * @brief Check if sensor configuration exists
     * @param name Sensor name
     * @return true if sensor exists in configuration
     */
    bool hasSensor(const std::string& name) const;

    /**
     * @brief Get the raw configuration JSON
     * @return const reference to the internal configuration
     */
    const nlohmann::json& getRawConfig() const { return config_; }

    /**
     * @brief Check if configuration has been loaded
     * @return true if configuration is loaded
     */
    bool isLoaded() const { return loaded_; }

private:
    nlohmann::json config_;
    bool loaded_ = false;
};

} // namespace roboclaw::hal
