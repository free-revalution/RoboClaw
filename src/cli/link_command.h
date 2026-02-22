#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace roboclaw::cli {

struct PlatformInfo {
    std::string id;
    std::string name;
    std::string description;
    bool enabled;
};

class LinkCommand {
public:
    LinkCommand();
    ~LinkCommand() = default;

    // Disable copy, enable move
    LinkCommand(const LinkCommand&) = delete;
    LinkCommand& operator=(const LinkCommand&) = delete;
    LinkCommand(LinkCommand&&) noexcept = default;
    LinkCommand& operator=(LinkCommand&&) noexcept = default;

    // Get available platforms
    std::vector<PlatformInfo> getAvailablePlatforms() const;

    // Validate platform configuration
    bool validatePlatformConfig(const std::string& platform_id,
                               const nlohmann::json& config) const;

    // Connect to platform
    bool connectToPlatform(const std::string& platform_id,
                          const nlohmann::json& config);

    // Disconnect from platform
    bool disconnectPlatform(const std::string& platform_id);

    // Get connection status
    std::string getConnectionStatus() const;

    // Save configuration
    bool saveConfig(const std::string& platform_id,
                   const nlohmann::json& config);

private:
    std::string config_file_path_;
};

} // namespace roboclaw::cli
