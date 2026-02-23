// plugins/vision/rplidar/rplidar_plugin.cpp
#include "rplidar_plugin.h"
#include <cstring>
#include <cmath>
#include <chrono>
#include <algorithm>

namespace roboclaw::plugins::vision {

RPLidarPlugin::RPLidarPlugin()
    : open_(false)
    , scanning_(false)
    , scan_frequency_(10)
    , baudrate_(115200)
    , scan_frequency_setting_(10) {
}

RPLidarPlugin::~RPLidarPlugin() {
    shutdown();
}

std::string RPLidarPlugin::getName() const {
    return "rplidar";
}

std::string RPLidarPlugin::getVersion() const {
    return "1.0.0";
}

bool RPLidarPlugin::initialize(const nlohmann::json& config) {
    std::lock_guard<std::mutex> lock(config_mutex_);

    // Parse configuration
    if (config.contains("port")) {
        serial_port_ = config["port"].get<std::string>();
    } else {
        serial_port_ = "/dev/ttyUSB0";  // Default
    }

    if (config.contains("baudrate")) {
        baudrate_ = config["baudrate"].get<int>();
    }

    if (config.contains("scan_frequency")) {
        scan_frequency_setting_ = config["scan_frequency"].get<int>();
    }

    // Initialize mock scan data
    initializeMockScan();

    // TODO: Initialize RPLIDAR SDK
    // - Create driver instance
    // - Connect to serial port
    // - Set scan frequency
    // - Start scanning

    return true;
}

void RPLidarPlugin::shutdown() {
    if (isStreaming()) {
        stopStream();
    }
    if (isOpen()) {
        closeDevice();
    }
}

bool RPLidarPlugin::openDevice(const std::string& config) {
    std::lock_guard<std::mutex> lock(config_mutex_);

    // TODO: Parse config and open actual device
    // - Parse port from config if provided
    // - Connect to RPLIDAR via serial
    // - Start motor
    // - Begin scanning

    open_ = true;
    return true;
}

void RPLidarPlugin::closeDevice() {
    std::lock_guard<std::mutex> lock(config_mutex_);

    if (isStreaming()) {
        stopStream();
    }

    // TODO: Stop and close RPLIDAR
    // - Stop scanning
    // - Stop motor
    // - Close serial port

    open_ = false;
}

FrameData RPLidarPlugin::captureFrame() {
    FrameData frame;

    if (!isOpen()) {
        return frame;
    }

    // Get current scan data
    ScanData scan = getScanData();

    // Convert scan to frame format
    return scanToFrame(scan);
}

void RPLidarPlugin::setParameter(const std::string& key, const nlohmann::json& value) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    params_[key] = value;

    // TODO: Apply parameter to actual device
    // if (key == "scan_frequency") { set frequency }
    // if (key == "angular_resolution") { set resolution }
}

nlohmann::json RPLidarPlugin::getParameter(const std::string& key) {
    std::lock_guard<std::mutex> lock(config_mutex_);

    if (params_.contains(key)) {
        return params_[key];
    }
    return nullptr;
}

nlohmann::json RPLidarPlugin::getDeviceCapabilities() {
    return {
        {"name", "Slamtec RPLIDAR"},
        {"model", "A2M8"},
        {"scan_frequency", {10, 15}},  // Hz
        {"max_distance", 12.0},  // meters
        {"angular_resolution", 1.0},  // degrees
        {"range", 0, 360},  // degrees
        {"ports", {"/dev/ttyUSB0", "/dev/ttyUSB1", "COM3", "COM4"}}
    };
}

void RPLidarPlugin::startStream(int fps) {
    std::lock_guard<std::mutex> lock(callback_mutex_);

    if (!isOpen() || isStreaming()) {
        return;
    }

    scan_frequency_ = fps;

    // TODO: Start actual RPLIDAR scanning
    // - Start scan mode
    // - Begin data acquisition

    scanning_ = true;

    // Start scan thread
    scan_thread_ = std::thread(&RPLidarPlugin::scanThread, this);
}

void RPLidarPlugin::stopStream() {
    std::lock_guard<std::mutex> lock(callback_mutex_);

    if (!isStreaming()) {
        return;
    }

    scanning_ = false;

    if (scan_thread_.joinable()) {
        scan_thread_.join();
    }

    // TODO: Stop actual RPLIDAR scanning
}

void RPLidarPlugin::registerFrameCallback(std::function<void(const FrameData&)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    frame_callback_ = callback;
}

bool RPLidarPlugin::isOpen() const {
    return open_.load();
}

bool RPLidarPlugin::isStreaming() const {
    return scanning_.load();
}

ScanData RPLidarPlugin::getScanData() {
    std::lock_guard<std::mutex> lock(scan_mutex_);

    // TODO: Get actual scan from RPLIDAR
    // - Wait for complete scan
    // - Extract all points
    // - Return scan data

    ScanData scan;
    scan.points = current_scan_.points;
    scan.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    scan.scan_count = 1;

    return scan;
}

nlohmann::json RPLidarPlugin::getObstacleDistances() {
    ScanData scan = getScanData();

    // Divide into 4 sectors: front (0), right (90), back (180), left (270)
    struct Sector {
        float min_distance = 100.0f;  // Initialize to large value
        int count = 0;
    };

    Sector sectors[4];

    for (const auto& point : scan.points) {
        if (!point.valid) continue;

        int sector = static_cast<int>(point.angle / 90.0) % 4;
        sectors[sector].min_distance = std::min(sectors[sector].min_distance, point.distance);
        sectors[sector].count++;
    }

    return {
        {"front", sectors[0].count > 0 ? sectors[0].min_distance : -1.0f},
        {"right", sectors[1].count > 0 ? sectors[1].min_distance : -1.0f},
        {"back", sectors[2].count > 0 ? sectors[2].min_distance : -1.0f},
        {"left", sectors[3].count > 0 ? sectors[3].min_distance : -1.0f}
    };
}

void RPLidarPlugin::initializeMockScan() {
    // Create 360 mock scan points (one per degree)
    mock_scan_points_.resize(MOCK_SCAN_POINTS);

    for (int i = 0; i < MOCK_SCAN_POINTS; ++i) {
        mock_scan_points_[i].angle = static_cast<float>(i);

        // Create some fake obstacles
        if ((i >= 0 && i < 45) || (i >= 315 && i < 360)) {
            // Front: obstacle at 1.2m
            mock_scan_points_[i].distance = 1.2f;
        } else if (i >= 45 && i < 135) {
            // Right: clear up to 5m
            mock_scan_points_[i].distance = 5.0f;
        } else if (i >= 135 && i < 225) {
            // Back: obstacle at 0.8m
            mock_scan_points_[i].distance = 0.8f;
        } else {
            // Left: obstacle at 0.5m
            mock_scan_points_[i].distance = 0.5f;
        }

        mock_scan_points_[i].quality = 200;
        mock_scan_points_[i].valid = true;
    }

    current_scan_.points = mock_scan_points_;
}

FrameData RPLidarPlugin::scanToFrame(const ScanData& scan) {
    FrameData frame;

    // For LiDAR, we represent the scan as a "frame" with metadata
    // The actual point data would typically be processed separately
    frame.width = scan.points.size();
    frame.height = 1;
    frame.channels = 1;  // Single channel: distance
    frame.stride = scan.points.size() * sizeof(float);
    frame.format = "LIDAR_SCAN";
    frame.timestamp = scan.timestamp;

    // Allocate buffer and copy point data
    size_t data_size = scan.points.size() * sizeof(ScanPoint);
    frame.data = new uint8_t[data_size];
    memcpy(frame.data, scan.points.data(), data_size);

    return frame;
}

void RPLidarPlugin::scanThread() {
    while (scanning_) {
        // Capture scan
        auto frame = captureFrame();

        // Deliver to callback
        std::function<void(const FrameData&)> callback;
        {
            std::lock_guard<std::mutex> lock(callback_mutex_);
            callback = frame_callback_;
        }

        if (callback) {
            callback(frame);
        }

        // Sleep to maintain scan frequency
        int scan_time_ms = 1000 / scan_frequency_.load();
        std::this_thread::sleep_for(std::chrono::milliseconds(scan_time_ms));
    }
}

// Factory functions
extern "C" {
    roboclaw::plugins::IVisionDevice* create() {
        return new RPLidarPlugin();
    }

    void destroy(roboclaw::plugins::IVisionDevice* plugin) {
        delete plugin;
    }
}

} // namespace roboclaw::plugins::vision
