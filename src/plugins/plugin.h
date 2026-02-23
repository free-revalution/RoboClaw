// src/plugins/plugin.h
#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace roboclaw::plugins {

/**
 * @brief Base interface for all RoboClaw plugins
 *
 * All plugins (vision devices, embedded platforms, simulation tools, etc.)
 * must inherit from this interface and implement its methods.
 */
class IPlugin {
public:
    virtual ~IPlugin() = default;

    /**
     * @brief Get the plugin name
     * @return Plugin name as a string
     */
    virtual std::string getName() const = 0;

    /**
     * @brief Get the plugin version
     * @return Version string (e.g., "1.0.0")
     */
    virtual std::string getVersion() const = 0;

    /**
     * @brief Initialize the plugin with configuration
     * @param config JSON configuration object
     * @return true if initialization succeeded
     * @throws std::runtime_error if configuration is invalid
     */
    virtual bool initialize(const nlohmann::json& config) = 0;

    /**
     * @brief Shutdown the plugin and release resources
     */
    virtual void shutdown() = 0;
};

/**
 * @brief Mock plugin for testing
 *
 * This plugin is used for unit testing the plugin interface.
 * It validates configuration by checking for a required "key" field.
 */
class MockPlugin : public IPlugin {
public:
    std::string getName() const override {
        return "mock";
    }

    std::string getVersion() const override {
        return "1.0.0";
    }

    bool initialize(const nlohmann::json& config) override {
        if (!config.contains("key")) {
            throw std::runtime_error("Invalid config: missing required 'key' field");
        }
        return true;
    }

    void shutdown() override {
        // No resources to release in mock
    }
};

} // namespace roboclaw::plugins
