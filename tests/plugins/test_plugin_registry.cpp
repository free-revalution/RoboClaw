// tests/plugins/test_plugin_registry.cpp
#include <catch2/catch.hpp>
#include "plugins/plugin_registry.h"
#include "plugins/plugin.h"

using namespace roboclaw::plugins;

TEST_CASE("Register and retrieve plugin", "[registry]") {
    PluginRegistry<IPlugin> registry;
    auto plugin = std::make_shared<MockPlugin>();

    REQUIRE(registry.registerPlugin("mock", plugin));
    auto retrieved = registry.getPlugin("mock");

    REQUIRE(retrieved != nullptr);
    REQUIRE(retrieved->getName() == "mock");
    REQUIRE(retrieved->getVersion() == "1.0.0");
}

TEST_CASE("Get non-existent plugin returns null", "[registry]") {
    PluginRegistry<IPlugin> registry;
    REQUIRE(registry.getPlugin("nonexistent") == nullptr);
}

TEST_CASE("List all registered plugins", "[registry]") {
    PluginRegistry<IPlugin> registry;
    registry.registerPlugin("mock1", std::make_shared<MockPlugin>());
    registry.registerPlugin("mock2", std::make_shared<MockPlugin>());

    auto plugins = registry.listPlugins();
    REQUIRE(plugins.size() == 2);

    // Sort for consistent ordering since unordered_map doesn't guarantee order
    std::sort(plugins.begin(), plugins.end());
    REQUIRE(plugins[0] == "mock1");
    REQUIRE(plugins[1] == "mock2");
}

TEST_CASE("Register duplicate plugin ID replaces existing", "[registry]") {
    PluginRegistry<IPlugin> registry;
    auto plugin1 = std::make_shared<MockPlugin>();
    auto plugin2 = std::make_shared<MockPlugin>();

    registry.registerPlugin("mock", plugin1);
    REQUIRE(registry.registerPlugin("mock", plugin2));

    auto retrieved = registry.getPlugin("mock");
    // The second registration should have replaced the first
    REQUIRE(retrieved == plugin2);
    REQUIRE(retrieved != plugin1);
}

TEST_CASE("List plugins returns empty when none registered", "[registry]") {
    PluginRegistry<IPlugin> registry;
    auto plugins = registry.listPlugins();
    REQUIRE(plugins.empty());
}

TEST_CASE("Unregister plugin", "[registry]") {
    PluginRegistry<IPlugin> registry;
    auto plugin = std::make_shared<MockPlugin>();

    registry.registerPlugin("mock", plugin);
    REQUIRE(registry.getPlugin("mock") != nullptr);

    registry.unregisterPlugin("mock");
    REQUIRE(registry.getPlugin("mock") == nullptr);
}

TEST_CASE("Unregister non-existent plugin is safe", "[registry]") {
    PluginRegistry<IPlugin> registry;
    // Should not throw or crash
    registry.unregisterPlugin("nonexistent");
    REQUIRE(registry.listPlugins().empty());
}

TEST_CASE("Register plugin with empty ID returns false", "[registry]") {
    PluginRegistry<IPlugin> registry;
    auto plugin = std::make_shared<MockPlugin>();
    REQUIRE_FALSE(registry.registerPlugin("", plugin));
}

TEST_CASE("Register null plugin returns false", "[registry]") {
    PluginRegistry<IPlugin> registry;
    std::shared_ptr<MockPlugin> null_plugin = nullptr;
    REQUIRE_FALSE(registry.registerPlugin("mock", null_plugin));
}

TEST_CASE("LoadPluginsFromDirectory stub logs not implemented", "[registry]") {
    PluginRegistry<IPlugin> registry;
    // This should not throw, just log "not implemented"
    registry.loadPluginsFromDirectory("/some/path");
    // No plugins should be loaded
    REQUIRE(registry.listPlugins().empty());
}
