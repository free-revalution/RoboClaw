// src/plugins/plugin_manager.cpp
#include "plugin_manager.h"

#include <stdexcept>
#include <functional>
#include <filesystem>
#include <iostream>
#include <sstream>

// Platform-specific headers for dynamic loading
#if defined(_WIN32) || defined(_WIN64)
    #ifndef PLATFORM_WINDOWS
        #define PLATFORM_WINDOWS
    #endif
    #include <windows.h>
#else
    #if defined(__APPLE__)
        #ifndef PLATFORM_MACOS
            #define PLATFORM_MACOS
        #endif
    #else
        #ifndef PLATFORM_LINUX
            #define PLATFORM_LINUX
        #endif
    #endif
    #include <dlfcn.h>
#endif

#include <sys/stat.h>

namespace roboclaw::plugins {

// ============================================================================
// Constructor/Destructor
// ============================================================================

PluginManager::~PluginManager() {
    shutdown();
}

// ============================================================================
// Public Methods
// ============================================================================

bool PluginManager::loadPlugin(const std::string& path) {
    // Validate inputs
    if (!validatePath(path)) {
        std::cerr << "PluginManager: Invalid plugin path '" << path << "'" << std::endl;
        return false;
    }

    // Check if file exists
    if (!fileExists(path)) {
        std::cerr << "PluginManager: Plugin file not found: '" << path << "'" << std::endl;
        return false;
    }

    // Extract plugin ID from path
    std::string id = extractPluginId(path);
    if (!validateId(id)) {
        std::cerr << "PluginManager: Invalid plugin ID extracted from path: '" << path << "'" << std::endl;
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    // Check if plugin with this ID is already loaded
    if (handles_.find(id) != handles_.end()) {
        // Plugin already loaded - replace it
        // Use unlocked version to avoid recursive deadlock since we already hold the lock
        std::cerr << "PluginManager: Plugin '" << id << "' already loaded, replacing..." << std::endl;
        (void)unloadPlugin_unlocked(id);
    }

    // Load the shared library
    void* handle = loadLibrary(path);
    if (!handle) {
        std::cerr << "PluginManager: Failed to load library '" << path << "': " << getDLError() << std::endl;
        return false;
    }

    // Look for the plugin factory function
    using CreatePluginFunc = IPlugin* (*)();
    auto* create_func = reinterpret_cast<CreatePluginFunc>(getSymbol(handle, "create_plugin"));

    if (!create_func) {
        std::cerr << "PluginManager: Plugin '" << id << "' missing 'create_plugin' symbol" << std::endl;
        unloadLibrary(handle);
        return false;
    }

    // Create the plugin instance
    IPlugin* plugin_raw = create_func();
    if (!plugin_raw) {
        std::cerr << "PluginManager: Plugin '" << id << "' factory returned null" << std::endl;
        unloadLibrary(handle);
        return false;
    }

    // Wrap in shared_ptr with custom deleter
    // NOTE: The deleter may run asynchronously from any thread when the last
    // shared_ptr reference is released. Plugin implementations must handle this.
    std::shared_ptr<IPlugin> plugin(plugin_raw, [id](IPlugin* p) {
        if (p) {
            p->shutdown();
            delete p;
        }
    });

    // Initialize the plugin with empty config by default
    nlohmann::json config;
    try {
        if (!plugin->initialize(config)) {
            std::cerr << "PluginManager: Plugin '" << id << "' initialization failed" << std::endl;
            unloadLibrary(handle);
            return false;
        }
    } catch (const std::exception& e) {
        // Initialization failed
        std::cerr << "PluginManager: Plugin '" << id << "' initialization threw exception: " << e.what() << std::endl;
        unloadLibrary(handle);
        return false;
    }

    // Register the plugin and store the handle
    if (!registry_.registerPlugin(id, plugin)) {
        std::cerr << "PluginManager: Failed to register plugin '" << id << "'" << std::endl;
        plugin->shutdown();
        unloadLibrary(handle);
        return false;
    }

    handles_[id] = handle;
    return true;
}

bool PluginManager::unloadPlugin(const std::string& id) {
    if (!validateId(id)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    return unloadPlugin_unlocked(id);
}

bool PluginManager::unloadPlugin_unlocked(const std::string& id) {
    // IMPORTANT: mutex_ must already be locked before calling this method

    auto handle_it = handles_.find(id);
    if (handle_it == handles_.end()) {
        return false;
    }

    // Unregister from registry (this will trigger plugin shutdown via shared_ptr deleter)
    registry_.unregisterPlugin(id);

    // Close the library handle
    unloadLibrary(handle_it->second);

    handles_.erase(handle_it);
    return true;
}

std::shared_ptr<IPlugin> PluginManager::getPlugin(const std::string& id) const {
    if (!validateId(id)) {
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    return registry_.getPlugin(id);
}

std::vector<std::string> PluginManager::listPlugins() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return registry_.listPlugins();
}

void PluginManager::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);

    // Unregister all plugins (this will trigger shutdown via shared_ptr deleters)
    registry_.clear();

    // Close all library handles
    for (auto& [id, handle] : handles_) {
        unloadLibrary(handle);
    }
    handles_.clear();
}

bool PluginManager::isLoaded(const std::string& id) const {
    if (!validateId(id)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    return handles_.find(id) != handles_.end();
}

size_t PluginManager::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return handles_.size();
}

bool PluginManager::empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return handles_.empty();
}

size_t PluginManager::loadPluginsFromDirectory(const std::string& directory) {
    namespace fs = std::filesystem;

    size_t loaded_count = 0;

    try {
        fs::path dir_path(directory);

        // Check if directory exists
        if (!fs::exists(dir_path) || !fs::is_directory(dir_path)) {
            return 0;
        }

        // Iterate through directory entries
        for (const auto& entry : fs::directory_iterator(dir_path)) {
            if (!entry.is_regular_file()) {
                continue;
            }

            std::string path = entry.path().string();

            // Check if file has valid library extension
            if (!validatePath(path)) {
                continue;
            }

            // Try to load the plugin
            if (loadPlugin(path)) {
                ++loaded_count;
            }
        }
    } catch (const fs::filesystem_error&) {
        // Directory access error - return count so far
        return loaded_count;
    } catch (const std::exception&) {
        // Other errors - return count so far
        return loaded_count;
    }

    return loaded_count;
}

// ============================================================================
// Private Helper Methods
// ============================================================================

bool PluginManager::validatePath(const std::string& path) const {
    if (path.empty()) {
        return false;
    }

    // Check for valid library extension based on platform
    #ifdef PLATFORM_WINDOWS
        if (path.size() < 4) return false;
        std::string ext = path.substr(path.size() - 4);
        // Case-insensitive check for .dll
        for (char& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return ext == ".dll";
    #elif defined(PLATFORM_MACOS)
        if (path.size() < 6) return false;
        return path.substr(path.size() - 6) == ".dylib";
    #else
        if (path.size() < 3) return false;
        return path.substr(path.size() - 3) == ".so";
    #endif
}

bool PluginManager::validateId(const std::string& id) const {
    return !id.empty();
}

std::string PluginManager::extractPluginId(const std::string& path) const {
    namespace fs = std::filesystem;

    try {
        fs::path p(path);
        // Get filename without extension
        std::string filename = p.stem().string();
        return filename;
    } catch (const fs::filesystem_error&) {
        // If path parsing fails, return empty string
        return "";
    } catch (const std::exception&) {
        return "";
    }
}

bool PluginManager::fileExists(const std::string& path) const {
    #ifdef PLATFORM_WINDOWS
        struct _stat buffer;
        return _stat(path.c_str(), &buffer) == 0;
    #else
        struct stat buffer;
        return stat(path.c_str(), &buffer) == 0;
    #endif
}

std::string PluginManager::getDLError() const {
    #ifdef PLATFORM_WINDOWS
        DWORD error = GetLastError();
        if (error == 0) {
            return "No error";
        }

        LPSTR messageBuffer = nullptr;
        size_t size = FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr,
            error,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPSTR>(&messageBuffer),
            0,
            nullptr
        );

        std::string message;
        if (size > 0 && messageBuffer) {
            message = std::string(messageBuffer, size);
        } else {
            message = "Unknown error (FormatMessageA failed)";
        }

        // Clean up the buffer allocated by FormatMessageA
        if (messageBuffer) {
            LocalFree(messageBuffer);
        }

        return message;
    #else
        const char* error = dlerror();
        return error ? error : "Unknown error";
    #endif
}

void* PluginManager::loadLibrary(const std::string& path) {
    #ifdef PLATFORM_WINDOWS
        HMODULE handle = LoadLibraryA(path.c_str());
        return reinterpret_cast<void*>(handle);
    #else
        // RTLD_LAZY: Resolve symbols lazily
        // RTLD_LOCAL: Symbols are not available to other objects
        void* handle = dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL);
        return handle;
    #endif
}

void PluginManager::unloadLibrary(void* handle) {
    if (!handle) {
        return;
    }

    #ifdef PLATFORM_WINDOWS
        FreeLibrary(reinterpret_cast<HMODULE>(handle));
    #else
        dlclose(handle);
    #endif
}

void* PluginManager::getSymbol(void* handle, const std::string& symbol) {
    if (!handle) {
        return nullptr;
    }

    #ifdef PLATFORM_WINDOWS
        FARPROC func = GetProcAddress(reinterpret_cast<HMODULE>(handle), symbol.c_str());
        return reinterpret_cast<void*>(func);
    #else
        // Clear any existing error
        dlerror();
        void* func = dlsym(handle, symbol.c_str());
        return func;
    #endif
}

} // namespace roboclaw::plugins
