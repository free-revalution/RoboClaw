// plugins/vision/realsense2/realsense2_plugin.cpp
#include "realsense2_plugin.h"
#include <cstring>
#include <chrono>
#include <thread>

namespace roboclaw::plugins::vision {

RealSense2Plugin::RealSense2Plugin()
    : open_(false)
    , streaming_(false)
    , fps_(30)
    , width_(640)
    , height_(480)
    , fps_setting_(30) {
}

RealSense2Plugin::~RealSense2Plugin() {
    shutdown();
}

std::string RealSense2Plugin::getName() const {
    return "realsense2";
}

std::string RealSense2Plugin::getVersion() const {
    return "1.0.0";
}

bool RealSense2Plugin::initialize(const nlohmann::json& config) {
    std::lock_guard<std::mutex> lock(config_mutex_);

    // Parse configuration
    if (config.contains("width")) {
        width_ = config["width"].get<int>();
    }
    if (config.contains("height")) {
        height_ = config["height"].get<int>();
    }
    if (config.contains("fps")) {
        fps_setting_ = config["fps"].get<int>();
    }

    // Initialize mock data
    initializeMockData();

    // TODO: Initialize RealSense SDK
    // - Create pipeline
    // - Configure streams
    // - Start device

    return true;
}

void RealSense2Plugin::shutdown() {
    if (isStreaming()) {
        stopStream();
    }
    if (isOpen()) {
        closeDevice();
    }
}

bool RealSense2Plugin::openDevice(const std::string& config) {
    std::lock_guard<std::mutex> lock(config_mutex_);

    // TODO: Parse config string and open actual device
    // - Parse serial number, device index, etc.
    // - Use rs2::context to find device
    // - Open device

    open_ = true;
    return true;
}

void RealSense2Plugin::closeDevice() {
    std::lock_guard<std::mutex> lock(config_mutex_);

    if (isStreaming()) {
        stopStream();
    }

    // TODO: Stop and close RealSense device
    // - Stop pipeline
    // - Release device resources

    open_ = false;
}

FrameData RealSense2Plugin::captureFrame() {
    FrameData frame;

    if (!isOpen()) {
        return frame;
    }

    std::lock_guard<std::mutex> lock(config_mutex_);

    // TODO: Capture actual frame from RealSense
    // - Wait for frames
    // - Get color/depth frame
    // - Copy frame data

    // For now, return mock data
    frame.width = width_;
    frame.height = height_;
    frame.channels = 3;  // RGB
    frame.stride = width_ * 3;
    frame.format = "RGB8";
    frame.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    // Allocate and copy mock data
    size_t data_size = width_ * height_ * 3;
    frame.data = new uint8_t[data_size];
    memcpy(frame.data, mock_frame_data_.data(), std::min(data_size, mock_frame_data_.size()));

    return frame;
}

void RealSense2Plugin::setParameter(const std::string& key, const nlohmann::json& value) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    params_[key] = value;

    // TODO: Apply parameter to actual device
    // if (key == "exposure") { set exposure value }
    // if (key == "gain") { set gain value }
    // if (key == "fps") { set FPS }
}

nlohmann::json RealSense2Plugin::getParameter(const std::string& key) {
    std::lock_guard<std::mutex> lock(config_mutex_);

    if (params_.contains(key)) {
        return params_[key];
    }
    return nullptr;
}

nlohmann::json RealSense2Plugin::getDeviceCapabilities() {
    return {
        {"name", "Intel RealSense Camera"},
        {"streams", {"color", "depth", "infrared1", "infrared2"}},
        {"resolutions", {
            {{"width", 640}, {"height", 480}},
            {{"width", 1280}, {"height", 720}},
            {{"width", 1920}, {"height", 1080}}
        }},
        {"fps_range", {1, 60}},
        {"formats", {"RGB8", "Z16", "Y8"}},
        {"depth_range", {0.0, 10.0}}  // meters
    };
}

void RealSense2Plugin::startStream(int fps) {
    std::lock_guard<std::mutex> lock(callback_mutex_);

    if (!isOpen() || isStreaming()) {
        return;
    }

    fps_ = fps;

    // TODO: Start actual RealSense streaming
    // - Start pipeline
    // - Begin frame acquisition

    streaming_ = true;

    // Start streaming thread
    stream_thread_ = std::thread(&RealSense2Plugin::streamingThread, this);
}

void RealSense2Plugin::stopStream() {
    std::lock_guard<std::mutex> lock(callback_mutex_);

    if (!isStreaming()) {
        return;
    }

    streaming_ = false;

    if (stream_thread_.joinable()) {
        stream_thread_.join();
    }

    // TODO: Stop actual RealSense streaming
    // - Stop pipeline
}

void RealSense2Plugin::registerFrameCallback(std::function<void(const FrameData&)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    frame_callback_ = callback;
}

bool RealSense2Plugin::isOpen() const {
    return open_.load();
}

bool RealSense2Plugin::isStreaming() const {
    return streaming_.load();
}

void RealSense2Plugin::initializeMockData() {
    // Create mock RGB frame data (gray pattern)
    size_t data_size = width_ * height_ * 3;
    mock_frame_data_.resize(data_size, 128);
}

void RealSense2Plugin::streamingThread() {
    while (streaming_) {
        // Capture frame
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

        // Sleep to maintain FPS
        int frame_time_ms = 1000 / fps_.load();
        std::this_thread::sleep_for(std::chrono::milliseconds(frame_time_ms));
    }
}

// Factory functions
extern "C" {
    roboclaw::plugins::IVisionDevice* create() {
        return new RealSense2Plugin();
    }

    void destroy(roboclaw::plugins::IVisionDevice* plugin) {
        delete plugin;
    }
}

} // namespace roboclaw::plugins::vision
