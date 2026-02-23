// src/plugins/plugin_registry.h
#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <stdexcept>

namespace roboclaw::plugins {

/**
 * @brief Thread-safe registry for managing plugin instances
 *
 * The PluginRegistry is a template class that manages plugin instances
 * of a specific type. It provides thread-safe operations for registering,
 * retrieving, listing, and unregistering plugins.
 *
 * @tparam T The base plugin interface type (e.g., IPlugin)
 */
template<typename T>
class PluginRegistry {
public:
    PluginRegistry() = default;
    ~PluginRegistry() = default;

    // Disable copy to avoid accidental duplication of plugin instances
    PluginRegistry(const PluginRegistry&) = delete;
    PluginRegistry& operator=(const PluginRegistry&) = delete;

    // Enable move
    PluginRegistry(PluginRegistry&&) = default;
    PluginRegistry& operator=(PluginRegistry&&) = default;

    /**
     * @brief Register a plugin with a unique ID
     *
     * If a plugin with the same ID already exists, it will be replaced.
     *
     * @param id Unique identifier for the plugin
     * @param plugin Shared pointer to the plugin instance
     * @return true if registration succeeded, false if id is empty or plugin is null
     */
    bool registerPlugin(const std::string& id, std::shared_ptr<T> plugin) {
        if (id.empty()) {
            return false;
        }

        if (!plugin) {
            return false;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        plugins_[id] = plugin;
        return true;
    }

    /**
     * @brief Retrieve a plugin by ID
     *
     * @param id The plugin identifier
     * @return Shared pointer to the plugin, or nullptr if not found
     */
    std::shared_ptr<T> getPlugin(const std::string& id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = plugins_.find(id);
        return (it != plugins_.end()) ? it->second : nullptr;
    }

    /**
     * @brief List all registered plugin IDs
     *
     * @return Vector of plugin IDs
     */
    std::vector<std::string> listPlugins() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> ids;
        ids.reserve(plugins_.size());
        for (const auto& [id, _] : plugins_) {
            ids.push_back(id);
        }
        return ids;
    }

    /**
     * @brief Unregister a plugin by ID
     *
     * If the plugin ID doesn't exist, this operation is a no-op.
     *
     * @param id The plugin identifier to remove
     */
    void unregisterPlugin(const std::string& id) {
        std::lock_guard<std::mutex> lock(mutex_);
        plugins_.erase(id);
    }

    /**
     * @brief Load plugins from a directory
     *
     * This is a stub method that logs "not implemented".
     * This will be implemented in Task 1.3 (Plugin Manager).
     *
     * @param path Path to the directory containing plugins
     */
    void loadPluginsFromDirectory(const std::string& path) {
        // TODO: Implement dynamic plugin loading in Task 1.3
        (void)path; // Suppress unused parameter warning
    }

    /**
     * @brief Get the number of registered plugins
     *
     * @return Number of plugins in the registry
     */
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return plugins_.size();
    }

    /**
     * @brief Check if the registry is empty
     *
     * @return true if no plugins are registered
     */
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return plugins_.empty();
    }

    /**
     * @brief Clear all plugins from the registry
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        plugins_.clear();
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<T>> plugins_;
};

} // namespace roboclaw::plugins
