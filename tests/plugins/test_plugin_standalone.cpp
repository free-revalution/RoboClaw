// Standalone test for plugin interface (when Catch2 is unavailable)
#include <iostream>
#include <cassert>
#include <nlohmann/json.hpp>
#include "plugins/plugin.h"

using namespace roboclaw::plugins;

int main() {
    std::cout << "Running standalone plugin interface tests..." << std::endl;

    // Test 1: Plugin provides name and version
    {
        MockPlugin plugin;
        assert(plugin.getName() == "mock");
        assert(plugin.getVersion() == "1.0.0");
        std::cout << "PASS: Plugin provides name and version" << std::endl;
    }

    // Test 2: Plugin initialize with valid config
    {
        MockPlugin plugin;
        nlohmann::json config = {{"key", "value"}};
        assert(plugin.initialize(config));
        std::cout << "PASS: Plugin initialize with valid config" << std::endl;
    }

    // Test 3: Plugin initialize with invalid config throws
    {
        MockPlugin plugin;
        nlohmann::json config;
        bool threw = false;
        try {
            plugin.initialize(config);
        } catch (const std::runtime_error&) {
            threw = true;
        }
        assert(threw);
        std::cout << "PASS: Plugin initialize with invalid config throws" << std::endl;
    }

    // Test 4: Plugin shutdown completes without throwing
    {
        MockPlugin plugin;
        nlohmann::json config = {{"key", "value"}};
        plugin.initialize(config);
        bool threw = false;
        try {
            plugin.shutdown();
        } catch (...) {
            threw = true;
        }
        assert(!threw);
        std::cout << "PASS: Plugin shutdown completes without throwing" << std::endl;
    }

    std::cout << "All tests passed!" << std::endl;
    return 0;
}
