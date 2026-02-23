// src/plugins/interfaces/ivision_device.h
#pragma once

#include <string>
#include <functional>
#include <nlohmann/json.hpp>
#include "../plugin.h"

namespace roboclaw::plugins {

/**
 * @brief Frame data structure for vision devices
 */
struct FrameData {
    void* data = nullptr;           // Pointer to frame data
    size_t width = 0;               // Frame width in pixels
    size_t height = 0;              // Frame height in pixels
    size_t channels = 0;            // Number of color channels (1=grayscale, 3=RGB, etc.)
    size_t stride = 0;              // Bytes per row
    int64_t timestamp = 0;          // Frame timestamp in microseconds
    std::string format;             // Pixel format (e.g., "RGB8", "YUYV", "BAYER_GR8")
};

/**
 * @brief Interface for vision device plugins
 *
 * Vision device plugins provide access to various vision sensors including
 * LiDAR, depth cameras, and industrial cameras.
 */
class IVisionDevice : public IPlugin {
public:
    virtual ~IVisionDevice() = default;

    /**
     * @brief Open the device with specified configuration
     * @param config Device configuration string or JSON
     * @return true if device opened successfully
     */
    virtual bool openDevice(const std::string& config) = 0;

    /**
     * @brief Close the device and release resources
     */
    virtual void closeDevice() = 0;

    /**
     * @brief Capture a single frame from the device
     * @return FrameData containing the captured frame
     */
    virtual FrameData captureFrame() = 0;

    /**
     * @brief Set a device parameter
     * @param key Parameter name (e.g., "exposure", "gain", "fps")
     * @param value Parameter value
     */
    virtual void setParameter(const std::string& key, const nlohmann::json& value) = 0;

    /**
     * @brief Get a device parameter
     * @param key Parameter name
     * @return Parameter value, or null if not found
     */
    virtual nlohmann::json getParameter(const std::string& key) = 0;

    /**
     * @brief Get device capabilities
     * @return JSON object describing device capabilities
     */
    virtual nlohmann::json getDeviceCapabilities() = 0;

    /**
     * @brief Start streaming mode at specified FPS
     * @param fps Frames per second for streaming (0 = max supported)
     */
    virtual void startStream(int fps) = 0;

    /**
     * @brief Stop streaming mode
     */
    virtual void stopStream() = 0;

    /**
     * @brief Register a callback for frame delivery in streaming mode
     * @param callback Function to call for each received frame
     */
    virtual void registerFrameCallback(std::function<void(const FrameData&)> callback) = 0;

    /**
     * @brief Check if device is currently open
     * @return true if device is open
     */
    virtual bool isOpen() const = 0;

    /**
     * @brief Check if device is currently streaming
     * @return true if streaming
     */
    virtual bool isStreaming() const = 0;
};

} // namespace roboclaw::plugins
