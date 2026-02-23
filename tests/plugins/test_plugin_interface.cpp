// tests/plugins/test_plugin_interface.cpp
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include "plugins/plugin.h"

using namespace roboclaw::plugins;

TEST(PluginInterfaceTest, ProvidesNameAndVersion) {
    MockPlugin plugin;
    EXPECT_EQ(plugin.getName(), "mock");
    EXPECT_EQ(plugin.getVersion(), "1.0.0");
}

TEST(PluginInterfaceTest, InitializeWithValidConfig) {
    MockPlugin plugin;
    nlohmann::json config = {{"key", "value"}};
    EXPECT_TRUE(plugin.initialize(config));
}

TEST(PluginInterfaceTest, InitializeWithInvalidConfigThrows) {
    MockPlugin plugin;
    nlohmann::json config;
    EXPECT_THROW(plugin.initialize(config), std::runtime_error);
}
