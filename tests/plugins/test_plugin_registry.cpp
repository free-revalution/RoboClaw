// tests/plugins/test_plugin_registry.cpp
#include <catch2/catch.hpp>
#include <thread>
#include <vector>
#include <atomic>
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

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST_CASE("Concurrent registration from multiple threads", "[registry][thread-safety]") {
    PluginRegistry<IPlugin> registry;
    constexpr int num_threads = 10;
    constexpr int plugins_per_thread = 100;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    std::atomic<int> failure_count{0};

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&registry, t, plugins_per_thread, &success_count, &failure_count]() {
            for (int i = 0; i < plugins_per_thread; ++i) {
                std::string id = "plugin_" + std::to_string(t) + "_" + std::to_string(i);
                auto plugin = std::make_shared<MockPlugin>();
                if (registry.registerPlugin(id, plugin)) {
                    ++success_count;
                } else {
                    ++failure_count;
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // All registrations should succeed
    REQUIRE(success_count == num_threads * plugins_per_thread);
    REQUIRE(failure_count == 0);

    // Verify all plugins are registered
    REQUIRE(registry.size() == static_cast<size_t>(num_threads * plugins_per_thread));

    // Verify each thread's plugins can be retrieved
    for (int t = 0; t < num_threads; ++t) {
        for (int i = 0; i < plugins_per_thread; ++i) {
            std::string id = "plugin_" + std::to_string(t) + "_" + std::to_string(i);
            auto plugin = registry.getPlugin(id);
            REQUIRE(plugin != nullptr);
            REQUIRE(plugin->getName() == "mock");
        }
    }
}

TEST_CASE("Concurrent retrieval from multiple threads", "[registry][thread-safety]") {
    PluginRegistry<IPlugin> registry;

    // First, register some plugins
    constexpr int num_plugins = 50;
    for (int i = 0; i < num_plugins; ++i) {
        std::string id = "plugin_" + std::to_string(i);
        registry.registerPlugin(id, std::make_shared<MockPlugin>());
    }

    constexpr int num_threads = 10;
    constexpr int retrievals_per_thread = 1000;
    std::vector<std::thread> threads;
    std::atomic<int> successful_retrievals{0};
    std::atomic<int> failed_retrievals{0};

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&registry, num_plugins, retrievals_per_thread, &successful_retrievals, &failed_retrievals]() {
            for (int i = 0; i < retrievals_per_thread; ++i) {
                // Pick a random plugin ID
                int plugin_idx = i % num_plugins;
                std::string id = "plugin_" + std::to_string(plugin_idx);
                auto plugin = registry.getPlugin(id);
                if (plugin != nullptr) {
                    ++successful_retrievals;
                } else {
                    ++failed_retrievals;
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // All retrievals should succeed
    REQUIRE(successful_retrievals == num_threads * retrievals_per_thread);
    REQUIRE(failed_retrievals == 0);
}

TEST_CASE("Concurrent mixed operations (register, retrieve, list)", "[registry][thread-safety]") {
    PluginRegistry<IPlugin> registry;
    constexpr int num_threads = 8;
    std::vector<std::thread> threads;
    std::atomic<bool> stop{false};
    std::atomic<int> register_count{0};
    std::atomic<int> retrieve_count{0};
    std::atomic<int> list_count{0};

    // Thread 0-2: Registration threads
    for (int t = 0; t < 3; ++t) {
        threads.emplace_back([&registry, t, &stop, &register_count]() {
            int counter = 0;
            while (!stop) {
                std::string id = "plugin_" + std::to_string(t) + "_" + std::to_string(counter++);
                auto plugin = std::make_shared<MockPlugin>();
                registry.registerPlugin(id, plugin);
                ++register_count;
                if (counter >= 100) break; // Limit registrations
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });
    }

    // Thread 3-5: Retrieval threads
    for (int t = 0; t < 3; ++t) {
        threads.emplace_back([&registry, &stop, &retrieve_count]() {
            while (!stop) {
                auto plugin = registry.getPlugin("plugin_0_0");
                if (plugin) {
                    ++retrieve_count;
                }
                std::this_thread::sleep_for(std::chrono::microseconds(50));
            }
        });
    }

    // Thread 6-7: List threads
    for (int t = 0; t < 2; ++t) {
        threads.emplace_back([&registry, &stop, &list_count]() {
            while (!stop) {
                auto plugins = registry.listPlugins();
                ++list_count;
                std::this_thread::sleep_for(std::chrono::microseconds(75));
            }
        });
    }

    // Let threads run for a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    stop = true;

    for (auto& thread : threads) {
        thread.join();
    }

    // Verify operations completed successfully
    REQUIRE(register_count > 0);
    REQUIRE(retrieve_count > 0);
    REQUIRE(list_count > 0);

    // Verify registry is still functional
    auto plugins = registry.listPlugins();
    REQUIRE(plugins.size() > 0);
}

TEST_CASE("Concurrent registration with duplicate IDs", "[registry][thread-safety]") {
    PluginRegistry<IPlugin> registry;
    constexpr int num_threads = 10;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    // All threads try to register with the same IDs
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&registry, &success_count]() {
            for (int i = 0; i < 10; ++i) {
                std::string id = "shared_plugin_" + std::to_string(i);
                auto plugin = std::make_shared<MockPlugin>();
                if (registry.registerPlugin(id, plugin)) {
                    ++success_count;
                }
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // All registrations should succeed (replacing is allowed)
    REQUIRE(success_count == num_threads * 10);

    // But we should only have 10 unique plugins
    REQUIRE(registry.size() == 10);
}

TEST_CASE("Concurrent unregister and retrieve", "[registry][thread-safety]") {
    PluginRegistry<IPlugin> registry;

    // Register initial plugins
    constexpr int num_plugins = 20;
    for (int i = 0; i < num_plugins; ++i) {
        std::string id = "plugin_" + std::to_string(i);
        registry.registerPlugin(id, std::make_shared<MockPlugin>());
    }

    constexpr int num_threads = 5;
    std::vector<std::thread> threads;
    std::atomic<bool> stop{false};
    std::atomic<int> unregister_count{0};
    std::atomic<int> retrieve_count{0};
    std::atomic<int> null_retrievals{0};

    // Half threads unregister, half retrieve
    for (int t = 0; t < num_threads; ++t) {
        if (t % 2 == 0) {
            // Unregister thread
            threads.emplace_back([&registry, &stop, &unregister_count]() {
                int counter = 0;
                while (!stop && counter < num_plugins) {
                    std::string id = "plugin_" + std::to_string(counter++);
                    registry.unregisterPlugin(id);
                    ++unregister_count;
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                }
            });
        } else {
            // Retrieve thread
            threads.emplace_back([&registry, &stop, &retrieve_count, &null_retrievals]() {
                int counter = 0;
                while (!stop) {
                    std::string id = "plugin_" + std::to_string(counter % num_plugins);
                    auto plugin = registry.getPlugin(id);
                    ++retrieve_count;
                    if (plugin == nullptr) {
                        ++null_retrievals;
                    }
                    ++counter;
                    std::this_thread::sleep_for(std::chrono::microseconds(50));
                }
            });
        }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    stop = true;

    for (auto& thread : threads) {
        thread.join();
    }

    // Verify operations completed
    REQUIRE(unregister_count > 0);
    REQUIRE(retrieve_count > 0);

    // Registry should be functional
    auto plugins = registry.listPlugins();
    REQUIRE(plugins.size() < num_plugins); // Some should have been unregistered
}

TEST_CASE("Stress test: Rapid concurrent operations", "[registry][thread-safety]") {
    PluginRegistry<IPlugin> registry;
    constexpr int num_threads = 20;
    constexpr int operations_per_thread = 500;
    std::vector<std::thread> threads;
    std::atomic<int> total_operations{0};

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&registry, t, operations_per_thread, &total_operations]() {
            for (int i = 0; i < operations_per_thread; ++i) {
                std::string id = "plugin_" + std::to_string((t * operations_per_thread + i) % 100);

                // Perform different operations based on iteration
                int op_type = i % 4;
                switch (op_type) {
                    case 0: // Register
                        registry.registerPlugin(id, std::make_shared<MockPlugin>());
                        break;
                    case 1: // Get
                        registry.getPlugin(id);
                        break;
                    case 2: // List
                        registry.listPlugins();
                        break;
                    case 3: // Unregister
                        registry.unregisterPlugin(id);
                        break;
                }
                ++total_operations;
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // Verify all operations completed without crash
    REQUIRE(total_operations == num_threads * operations_per_thread);

    // Registry should still be functional
    auto plugins = registry.listPlugins();
    // At minimum, we should be able to list plugins
    REQUIRE(plugins.size() < 1000); // Sanity check
}
