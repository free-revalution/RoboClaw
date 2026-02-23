// tests/plugins/test_plugin_interface.cpp
#include <catch2/catch.hpp>
#include "plugins/plugin.h"
#include "test_plugin_utils.h"

using namespace roboclaw::plugins;
using namespace roboclaw::plugins::test;

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

TEST_CASE("Plugin shutdown completes without throwing", "[plugin]") {
    MockPlugin plugin;
    nlohmann::json config = {{"key", "value"}};
    plugin.initialize(config);
    REQUIRE_NOTHROW(plugin.shutdown());
}
