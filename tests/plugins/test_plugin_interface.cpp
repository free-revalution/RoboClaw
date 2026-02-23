// tests/plugins/test_plugin_interface.cpp
#include <catch2/catch.hpp>
#include "plugins/plugin.h"

using namespace roboclaw::plugins;

TEST_CASE("Plugin interface provides name and version", "[plugin]") {
    MockPlugin plugin;
    REQUIRE(plugin.getName() == "mock");
    REQUIRE(plugin.getVersion() == "1.0.0");
}

TEST_CASE("Plugin initialize with valid config", "[plugin]") {
    MockPlugin plugin;
    nlohmann::json config = {{"key", "value"}};
    REQUIRE(plugin.initialize(config));
}

TEST_CASE("Plugin initialize with invalid config throws", "[plugin]") {
    MockPlugin plugin;
    nlohmann::json config;
    REQUIRE_THROWS_AS(plugin.initialize(config), std::runtime_error);
}
