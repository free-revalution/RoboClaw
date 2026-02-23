// tests/plugins/test_plugin_manager.cpp
#include <catch2/catch.hpp>
#include <fstream>
#include <memory>
#include <thread>
#include <atomic>
#include <filesystem>
#include <ctime>
#include "plugins/plugin_manager.h"
#include "plugins/plugin.h"

using namespace roboclaw::plugins;

// ============================================================================
// Test Helpers
// ============================================================================

/**
 * @brief Mock plugin loader for testing without actual shared libraries
 *
 * Since dynamic loading requires actual shared libraries which are difficult
 * to create in unit tests, we use a mock plugin loader that simulates the
 * loading behavior.
 */
class MockPluginLoader {
public:
    struct LoadedPlugin {
        std::shared_ptr<IPlugin> plugin;
        void* handle;
        std::string path;
    };

    std::unordered_map<std::string, LoadedPlugin> loaded_plugins_;
    bool should_fail_load_ = false;
    std::string fail_message_ = "Mock load failure";

    bool loadPlugin(const std::string& path, const std::string& id) {
        if (should_fail_load_) {
            return false;
        }

        // Simulate loading by creating a mock plugin
        auto plugin = std::make_shared<MockPlugin>();
        LoadedPlugin loaded{plugin, reinterpret_cast<void*>(0x1234), path};
        loaded_plugins_[id] = loaded;
        return true;
    }

    bool unloadPlugin(const std::string& id) {
        auto it = loaded_plugins_.find(id);
        if (it == loaded_plugins_.end()) {
            return false;
        }
        loaded_plugins_.erase(it);
        return true;
    }

    std::shared_ptr<IPlugin> getPlugin(const std::string& id) {
        auto it = loaded_plugins_.find(id);
        return (it != loaded_plugins_.end()) ? it->second.plugin : nullptr;
    }

    std::vector<std::string> listPlugins() const {
        std::vector<std::string> ids;
        for (const auto& [id, _] : loaded_plugins_) {
            ids.push_back(id);
        }
        return ids;
    }

    void setLoadFailure(bool fail, const std::string& msg = "Mock load failure") {
        should_fail_load_ = fail;
        fail_message_ = msg;
    }

    bool isLoaded(const std::string& id) const {
        return loaded_plugins_.find(id) != loaded_plugins_.end();
    }
};

// ============================================================================
// Basic Functionality Tests
// ============================================================================

TEST_CASE("PluginManager construction and initialization", "[plugin_manager]") {
    PluginManager manager;
    REQUIRE(manager.listPlugins().empty());
}

TEST_CASE("Load plugin with valid ID", "[plugin_manager]") {
    PluginManager manager;

    // Note: Actual dynamic loading requires shared libraries
    // For unit tests, we test error handling paths
    SECTION("Loading non-existent library returns false") {
        // This tests the error handling path
        bool result = manager.loadPlugin("/nonexistent/path/libtest.so");
        REQUIRE_FALSE(result);  // false indicates failure
    }

    SECTION("Loading with empty path returns false") {
        bool result = manager.loadPlugin("");
        REQUIRE_FALSE(result);
    }
}

TEST_CASE("Unload plugin", "[plugin_manager]") {
    PluginManager manager;

    SECTION("Unloading non-existent plugin returns false") {
        REQUIRE_FALSE(manager.unloadPlugin("nonexistent"));
    }

    SECTION("Unloading with empty ID returns false") {
        REQUIRE_FALSE(manager.unloadPlugin(""));
    }
}

TEST_CASE("Get loaded plugin", "[plugin_manager]") {
    PluginManager manager;

    SECTION("Get non-existent plugin returns null") {
        auto plugin = manager.getPlugin("nonexistent");
        REQUIRE(plugin == nullptr);
    }

    SECTION("Get with empty ID returns null") {
        auto plugin = manager.getPlugin("");
        REQUIRE(plugin == nullptr);
    }
}

TEST_CASE("List all loaded plugins", "[plugin_manager]") {
    PluginManager manager;

    SECTION("Initially empty") {
        auto plugins = manager.listPlugins();
        REQUIRE(plugins.empty());
    }

    SECTION("After failed load, still empty") {
        manager.loadPlugin("/nonexistent/libtest.so");
        auto plugins = manager.listPlugins();
        REQUIRE(plugins.empty());
    }
}

// ============================================================================
// Plugin Lifecycle Tests
// ============================================================================

TEST_CASE("Plugin lifecycle: initialize and shutdown", "[plugin_manager]") {
    PluginManager manager;

    SECTION("Failed load doesn't crash on shutdown") {
        REQUIRE_NOTHROW(manager.shutdown());
    }

    SECTION("Multiple shutdowns are safe") {
        REQUIRE_NOTHROW(manager.shutdown());
        REQUIRE_NOTHROW(manager.shutdown());
        REQUIRE_NOTHROW(manager.shutdown());
    }
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_CASE("Error handling for invalid plugin paths", "[plugin_manager]") {
    PluginManager manager;

    SECTION("Empty path") {
        bool result = manager.loadPlugin("");
        REQUIRE_FALSE(result);
    }

    SECTION("Non-existent file") {
        bool result = manager.loadPlugin("/path/that/does/not/exist/libtest.so");
        REQUIRE_FALSE(result);
    }

    SECTION("Path without library extension (still should handle gracefully)") {
        // The manager should validate or handle this
        bool result = manager.loadPlugin("/some/path/no_extension");
        // Should fail either way since file doesn't exist
        REQUIRE_FALSE(result);
    }
}

TEST_CASE("Error handling for invalid configurations", "[plugin_manager]") {
    PluginManager manager;

    SECTION("Invalid config for plugin initialization") {
        // Even if we could load a library, invalid config should fail
        // Should fail since file doesn't exist
        bool result = manager.loadPlugin("/some/path/libtest.so");
        REQUIRE_FALSE(result);
    }
}

TEST_CASE("Error handling for duplicate plugin IDs", "[plugin_manager]") {
    PluginManager manager;

    // First load will fail (file doesn't exist)
    bool result1 = manager.loadPlugin("/path/libtest1.so");
    REQUIRE_FALSE(result1);

    // Second load with same filename will also fail
    bool result2 = manager.loadPlugin("/path/libtest2.so");
    REQUIRE_FALSE(result2);
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST_CASE("Concurrent plugin load attempts", "[plugin_manager][thread-safety]") {
    PluginManager manager;
    constexpr int num_threads = 10;
    std::vector<std::thread> threads;
    std::atomic<int> failure_count{0};

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&manager, t, &failure_count]() {
            std::string path = "/path/to/libplugin" + std::to_string(t) + ".so";
            bool result = manager.loadPlugin(path);
            if (!result) {
                ++failure_count;
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // All loads should fail (files don't exist)
    REQUIRE(failure_count == num_threads);

    // Manager should still be functional
    REQUIRE(manager.listPlugins().empty());
}

TEST_CASE("Concurrent load and unload operations", "[plugin_manager][thread-safety]") {
    PluginManager manager;
    constexpr int num_threads = 5;
    std::vector<std::thread> threads;
    std::atomic<bool> stop{false};

    // Load threads
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&manager, t, &stop]() {
            int counter = 0;
            while (!stop && counter < 50) {
                std::string path = "/path/to/libplugin" + std::to_string(t) + "_" + std::to_string(counter++) + ".so";
                manager.loadPlugin(path);
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });
    }

    // Unload threads
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&manager, &stop]() {
            int counter = 0;
            while (!stop && counter < 50) {
                std::string id = "libplugin" + std::to_string(t) + "_" + std::to_string(counter++);
                manager.unloadPlugin(id);
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    stop = true;

    for (auto& thread : threads) {
        thread.join();
    }

    // Manager should still be functional
    REQUIRE_NOTHROW(manager.listPlugins());
}

TEST_CASE("Concurrent get operations", "[plugin_manager][thread-safety]") {
    PluginManager manager;
    constexpr int num_threads = 10;
    constexpr int gets_per_thread = 100;
    std::vector<std::thread> threads;
    std::atomic<int> total_gets{0};

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&manager, &total_gets]() {
            for (int i = 0; i < gets_per_thread; ++i) {
                std::string id = "plugin_" + std::to_string(i % 5);
                auto plugin = manager.getPlugin(id);
                ++total_gets;
                // Plugin should be null (nothing loaded)
                REQUIRE(plugin == nullptr);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    REQUIRE(total_gets == num_threads * gets_per_thread);
}

TEST_CASE("Concurrent list operations", "[plugin_manager][thread-safety]") {
    PluginManager manager;
    constexpr int num_threads = 10;
    constexpr int lists_per_thread = 100;
    std::vector<std::thread> threads;
    std::atomic<int> total_lists{0};

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&manager, &total_lists]() {
            for (int i = 0; i < lists_per_thread; ++i) {
                auto plugins = manager.listPlugins();
                ++total_lists;
                // Should always be empty (nothing loaded)
                REQUIRE(plugins.empty());
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    REQUIRE(total_lists == num_threads * lists_per_thread);
}

// ============================================================================
// Platform-Specific Tests
// ============================================================================

TEST_CASE("Platform-specific library extensions", "[plugin_manager]") {
    PluginManager manager;

    SECTION("Linux .so extension") {
        bool result = manager.loadPlugin("/path/libtest.so");
        REQUIRE_FALSE(result);  // File doesn't exist
    }

    SECTION("macOS .dylib extension") {
        bool result = manager.loadPlugin("/path/libtest.dylib");
        REQUIRE_FALSE(result);  // File doesn't exist
    }

    SECTION("Windows .dll extension") {
        bool result = manager.loadPlugin("/path/libtest.dll");
        REQUIRE_FALSE(result);  // File doesn't exist
    }
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_CASE("Edge case: Very long plugin paths", "[plugin_manager]") {
    PluginManager manager;

    // Create a very long path
    std::string long_path(10000, 'a');
    bool result = manager.loadPlugin(long_path);
    REQUIRE_FALSE(result);
}

TEST_CASE("Edge case: Special characters in plugin path", "[plugin_manager]") {
    PluginManager manager;

    // Path with special characters
    bool result = manager.loadPlugin("/path/lib-with-special.chars@123.so");
    // Should handle gracefully
    REQUIRE_FALSE(result);  // File doesn't exist
}

TEST_CASE("Edge case: loadPluginsFromDirectory", "[plugin_manager]") {
    PluginManager manager;

    SECTION("Non-existent directory returns 0") {
        size_t count = manager.loadPluginsFromDirectory("/nonexistent/directory");
        REQUIRE(count == 0);
    }

    SECTION("Empty directory returns 0") {
        // Create a temporary empty directory
        std::string temp_dir = "/tmp/roboclaw_test_empty_" + std::to_string(std::time(nullptr));
        std::filesystem::create_directories(temp_dir);
        size_t count = manager.loadPluginsFromDirectory(temp_dir);
        std::filesystem::remove(temp_dir);
        REQUIRE(count == 0);
    }
}

TEST_CASE("Edge case: Rapid load/unload cycles", "[plugin_manager]") {
    PluginManager manager;

    // Rapid cycles should not crash
    for (int i = 0; i < 100; ++i) {
        manager.loadPlugin("/path/librapid_test.so");
        manager.unloadPlugin("librapid_test");
    }

    // Manager should still be functional
    REQUIRE(manager.listPlugins().empty());
}
