// tests/plugins/test_plugin_utils.h
#pragma once

#include "plugins/plugin.h"
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <memory>

namespace roboclaw::plugins::test {

/**
 * @brief Mock plugin for testing
 *
 * This plugin is used for unit testing the plugin interface.
 * It validates configuration by checking for a required "key" field.
 *
 * This is intentionally in a separate test header to avoid polluting
 * the production plugin.h interface with test-only code.
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

/**
 * @brief Factory function for creating MockPlugin instances
 *
 * This matches the expected signature for plugin factory functions.
 *
 * @return Pointer to a new MockPlugin instance
 */
inline IPlugin* createMockPlugin() {
    return new MockPlugin();
}

/**
 * @brief Helper to create a shared_ptr to a MockPlugin
 *
 * @return Shared pointer to a new MockPlugin instance
 */
inline std::shared_ptr<IPlugin> createMockPluginShared() {
    return std::make_shared<MockPlugin>();
}

} // namespace roboclaw::plugins::test
