// plugins/vision/realsense2/realsense2_plugin.h
#pragma once

#include "../../../src/plugins/interfaces/ivision_device.h"
#include <memory>
#include <mutex>
#include <atomic>
#include <thread>
#include <queue>

namespace roboclaw::plugins::vision {

/**
 * @brief RealSense2 camera plugin
 *
 * This plugin provides support for Intel RealSense cameras (D400 series, etc.)
 * including color, depth, and infrared streams.
 *
 * NOTE: This is a skeleton implementation. Full SDK integration requires:
 * - librealsense2 library installation
 * - Device enumeration and configuration
 * - Frame acquisition from camera
 */
class RealSense2Plugin : public roboclaw::plugins::IVisionDevice {
public:
    RealSense2Plugin();
    ~RealSense2Plugin() override;

    // IPlugin interface
    std::string getName() const override;
    std::string getVersion() const override;
    bool initialize(const nlohmann::json& config) override;
    void shutdown() override;

    // IVisionDevice interface
    bool openDevice(const std::string& config) override;
    void closeDevice() override;
    FrameData captureFrame() override;
    void setParameter(const std::string& key, const nlohmann::json& value) override;
    nlohmann::json getParameter(const std::string& key) override;
    nlohmann::json getDeviceCapabilities() override;
    void startStream(int fps) override;
    void stopStream() override;
    void registerFrameCallback(std::function<void(const FrameData&)> callback) override;
    bool isOpen() const override;
    bool isStreaming() const override;

private:
    /**
     * @brief Initialize mock frame data for testing
     */
    void initializeMockData();

    /**
     * @brief Streaming thread function
     */
    void streamingThread();

    // Device state
    std::atomic<bool> open_;
    std::atomic<bool> streaming_;
    std::atomic<int> fps_;

    // Configuration
    int width_;
    int height_;
    int fps_setting_;

    // Frame storage for callback delivery
    std::function<void(const FrameData&)> frame_callback_;

    // Streaming thread
    std::thread stream_thread_;

    // Mock data for testing (will be replaced with actual SDK calls)
    std::vector<uint8_t> mock_frame_data_;

    // Mutex for thread safety
    mutable std::mutex config_mutex_;
    mutable std::mutex callback_mutex_;

    // Parameters
    nlohmann::json params_;
};

/**
 * @brief Factory function for plugin loading
 *
 * This function is exported and called by the PluginManager
 * when loading the RealSense2 plugin.
 */
extern "C" {
    roboclaw::plugins::IVisionDevice* create();
    void destroy(roboclaw::plugins::IVisionDevice* plugin);
}

} // namespace roboclaw::plugins::vision
