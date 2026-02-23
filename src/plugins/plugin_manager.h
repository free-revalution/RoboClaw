// src/plugins/plugin_manager.h
#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <stdexcept>
#include <filesystem>

#include "plugin_registry.h"
#include "plugin.h"

namespace roboclaw::plugins {

/**
 * @brief Manages dynamic loading and lifecycle of plugins
 *
 * The PluginManager is responsible for loading plugins from shared libraries
 * (.so on Linux, .dylib on macOS, .dll on Windows), managing their lifecycle,
 * and integrating with the PluginRegistry for plugin management.
 *
 * Key features:
 * - Dynamic plugin loading using dlopen/dlsym
 * - Thread-safe operations
 * - Plugin lifecycle management (initialize, shutdown, unload)
 * - Error handling for load failures
 * - Platform-specific library extension handling
 */
class PluginManager {
public:
    PluginManager() = default;
    ~PluginManager();

    // Disable copy to avoid issues with loaded library handles
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;

    // Disable move (mutex is not movable)
    PluginManager(PluginManager&&) = delete;
    PluginManager& operator=(PluginManager&&) = delete;

    /**
     * @brief Load a plugin from a shared library
     *
     * Loads a plugin from the specified path. The plugin ID is extracted from
     * the library filename (without extension). The plugin must export a factory
     * function called "create_plugin" that returns an IPlugin*.
     *
     * @param path Path to the shared library (.so/.dylib/.dll)
     * @return true if the plugin was loaded successfully, false otherwise
     */
    [[nodiscard]] bool loadPlugin(const std::string& path);

    /**
     * @brief Load all plugins from a directory
     *
     * Scans the specified directory for shared libraries (.so/.dylib/.dll)
     * and attempts to load each one. The plugin ID is extracted from the
     * library filename (without extension).
     *
     * @param directory Path to the directory containing plugin libraries
     * @return Number of plugins successfully loaded
     */
    [[nodiscard]] size_t loadPluginsFromDirectory(const std::string& directory);

    /**
     * @brief Unload a plugin by ID
     *
     * Shuts down the plugin and unloads the shared library.
     * If the plugin ID doesn't exist, this operation is a no-op.
     *
     * @param id The plugin identifier to unload
     * @return true if the plugin was unloaded, false if not found
     */
    [[nodiscard]] bool unloadPlugin(const std::string& id);

    /**
     * @brief Get a loaded plugin by ID
     *
     * @param id The plugin identifier
     * @return Shared pointer to the plugin, or nullptr if not found
     */
    [[nodiscard]] std::shared_ptr<IPlugin> getPlugin(const std::string& id) const;

    /**
     * @brief List all loaded plugin IDs
     *
     * @return Vector of plugin IDs
     */
    [[nodiscard]] std::vector<std::string> listPlugins() const;

    /**
     * @brief Shutdown all loaded plugins and release resources
     *
     * Unloads all plugins and closes all shared library handles.
     * After calling this method, the manager is in its initial state.
     */
    void shutdown();

    /**
     * @brief Check if a plugin is loaded
     *
     * @param id The plugin identifier
     * @return true if the plugin is currently loaded
     */
    [[nodiscard]] bool isLoaded(const std::string& id) const;

    /**
     * @brief Get the number of loaded plugins
     *
     * @return Number of loaded plugins
     */
    [[nodiscard]] size_t size() const;

    /**
     * @brief Check if the manager has no loaded plugins
     *
     * @return true if no plugins are loaded
     */
    [[nodiscard]] bool empty() const;

private:
    /**
     * @brief Validate a plugin path
     *
     * Checks if the path is non-empty and has a valid library extension.
     *
     * @param path The path to validate
     * @return true if the path is valid
     */
    [[nodiscard]] bool validatePath(const std::string& path) const;

    /**
     * @brief Validate a plugin ID
     *
     * Checks if the ID is non-empty.
     *
     * @param id The ID to validate
     * @return true if the ID is valid
     */
    [[nodiscard]] bool validateId(const std::string& id) const;

    /**
     * @brief Extract plugin ID from library path
     *
     * Extracts the plugin ID from the library filename by removing
     * the path and extension. For example:
     * - "/path/to/libmyplugin.so" -> "libmyplugin"
     * - "/path/to/myplugin.dylib" -> "myplugin"
     *
     * @param path Path to the shared library
     * @return Extracted plugin ID
     */
    [[nodiscard]] std::string extractPluginId(const std::string& path) const;

    /**
     * @brief Get the last dynamic loading error
     *
     * Returns a human-readable error message from the last dlopen/dlsym call.
     *
     * @return Error message string
     */
    [[nodiscard]] std::string getDLError() const;

    /**
     * @brief Check if a file exists
     *
     * @param path Path to the file
     * @return true if the file exists
     */
    [[nodiscard]] bool fileExists(const std::string& path) const;

    /**
     * @brief Internal unload helper that assumes mutex is already held
     *
     * This is a private helper that performs the actual unload operation
     * without acquiring the mutex. It is used by loadPlugin() to avoid
     * recursive deadlock when replacing an already-loaded plugin.
     *
     * IMPORTANT: The mutex_ must be locked before calling this method.
     *
     * @param id The plugin identifier to unload
     * @return true if the plugin was unloaded, false if not found
     */
    [[nodiscard]] bool unloadPlugin_unlocked(const std::string& id);

    /**
     * @brief Platform-specific plugin loading implementation
     *
     * @param path Path to the shared library
     * @return Handle to the loaded library, or nullptr on failure
     */
    [[nodiscard]] void* loadLibrary(const std::string& path);

    /**
     * @brief Platform-specific plugin unloading implementation
     *
     * @param handle Handle to the loaded library
     */
    void unloadLibrary(void* handle);

    /**
     * @brief Platform-specific symbol lookup implementation
     *
     * @param handle Handle to the loaded library
     * @param symbol Name of the symbol to find
     * @return Pointer to the symbol, or nullptr if not found
     */
    [[nodiscard]] void* getSymbol(void* handle, const std::string& symbol);

    // Plugin registry for managing plugin instances
    PluginRegistry<IPlugin> registry_;

    // Map of plugin IDs to their library handles
    std::unordered_map<std::string, void*> handles_;

    // Mutex for thread safety
    mutable std::mutex mutex_;
};

} // namespace roboclaw::plugins
