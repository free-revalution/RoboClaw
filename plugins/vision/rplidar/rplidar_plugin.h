// plugins/vision/rplidar/rplidar_plugin.h
#pragma once

#include "../../../src/plugins/interfaces/ivision_device.h"
#include <memory>
#include <mutex>
#include <atomic>
#include <thread>
#include <vector>

namespace roboclaw::plugins::vision {

/**
 * @brief LiDAR scan data point
 */
struct ScanPoint {
    float angle;      // Angle in degrees (0-360)
    float distance;   // Distance in meters
    int quality;      // Signal quality (0-255)
    bool valid;       // Whether the point is valid
};

/**
 * @brief Complete scan from LiDAR
 */
struct ScanData {
    std::vector<ScanPoint> points;
    int64_t timestamp;
    int scan_count;  // Number of complete scans
};

/**
 * @brief RPLIDAR plugin
 *
 * This plugin provides support for Slamtec RPLIDAR devices (A1, A2, A3 series)
 * for 2D laser scanning and SLAM applications.
 *
 * NOTE: This is a skeleton implementation. Full SDK integration requires:
 * - rplidar_sdk library installation
 * - Serial communication with LiDAR
 * - Scan data acquisition and processing
 */
class RPLidarPlugin : public roboclaw::plugins::IVisionDevice {
public:
    RPLidarPlugin();
    ~RPLidarPlugin() override;

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

    /**
     * @brief Get complete scan data
     * @return Scan data with all points
     */
    ScanData getScanData();

    /**
     * @brief Get obstacle distances in 4 directions
     * @return JSON with front, back, left, right distances
     */
    nlohmann::json getObstacleDistances();

private:
    /**
     * @brief Initialize mock scan data
     */
    void initializeMockScan();

    /**
     * @brief Scan thread function
     */
    void scanThread();

    /**
     * @brief Convert scan data to frame format
     * @param scan Scan data
     * @return Frame data representation
     */
    FrameData scanToFrame(const ScanData& scan);

    // Device state
    std::atomic<bool> open_;
    std::atomic<bool> scanning_;
    std::atomic<int> scan_frequency_;

    // Configuration
    std::string serial_port_;
    int baudrate_;
    int scan_frequency_setting_;

    // Scan data
    ScanData current_scan_;

    // Callback
    std::function<void(const FrameData&)> frame_callback_;

    // Scan thread
    std::thread scan_thread_;

    // Mock data for testing
    static constexpr int MOCK_SCAN_POINTS = 360;
    std::vector<ScanPoint> mock_scan_points_;

    // Mutex
    mutable std::mutex config_mutex_;
    mutable std::mutex callback_mutex_;
    mutable std::mutex scan_mutex_;

    // Parameters
    nlohmann::json params_;
};

/**
 * @brief Factory function for plugin loading
 */
extern "C" {
    roboclaw::plugins::IVisionDevice* create();
    void destroy(roboclaw::plugins::IVisionDevice* plugin);
}

} // namespace roboclaw::plugins::vision
