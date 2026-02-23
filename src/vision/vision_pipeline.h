// src/vision/vision_pipeline.h
#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <mutex>
#include <atomic>
#include <chrono>
#include "plugins/interfaces/ivision_device.h"
#include "frame_processor.h"

namespace roboclaw::vision {

/**
 * @brief Pipeline operation modes
 */
enum class PipelineMode {
    REALTIME,   // Real-time perception for SLAM/Navigation
    DETECTION,  // Object detection and recognition
    RECORDING   // Data recording for offline analysis
};

/**
 * @brief Frame data structure (compatible with IVisionDevice::FrameData)
 */
struct FrameData {
    void* data = nullptr;           // Pointer to frame data
    size_t width = 0;               // Frame width in pixels
    size_t height = 0;              // Frame height in pixels
    size_t channels = 0;            // Number of color channels
    size_t stride = 0;              // Bytes per row
    int64_t timestamp = 0;          // Frame timestamp in microseconds
    std::string format;             // Pixel format (e.g., "RGB8", "YUYV")

    // Helper for cleanup
    ~FrameData() {
        if (data) {
            delete[] static_cast<uint8_t*>(data);
            data = nullptr;
        }
    }

    // Copy constructor
    FrameData(const FrameData& other) : width(other.width), height(other.height),
                                         channels(other.channels), stride(other.stride),
                                         timestamp(other.timestamp), format(other.format) {
        if (other.data && other.width * other.height * channels > 0) {
            data = new uint8_t[width * height * channels];
            memcpy(data, other.data, width * height * channels);
        }
    }

    // Move constructor
    FrameData(FrameData&& other) noexcept
        : data(other.data), width(other.width), height(other.height),
          channels(other.channels), stride(other.stride),
          timestamp(other.timestamp), format(std::move(other.format)) {
        other.data = nullptr;
    }

    // Assignment operator
    FrameData& operator=(const FrameData& other) {
        if (this != &other) {
            if (data) delete[] static_cast<uint8_t*>(data);

            width = other.width;
            height = other.height;
            channels = other.channels;
            stride = other.stride;
            timestamp = other.timestamp;
            format = other.format;

            if (other.data && other.width * other.height * channels > 0) {
                data = new uint8_t[width * height * channels];
                memcpy(data, other.data, width * height * channels);
            } else {
                data = nullptr;
            }
        }
        return *this;
    }

    // Move assignment
    FrameData& operator=(FrameData&& other) noexcept {
        if (this != &other) {
            if (data) delete[] static_cast<uint8_t*>(data);

            data = other.data;
            width = other.width;
            height = other.height;
            channels = other.channels;
            stride = other.stride;
            timestamp = other.timestamp;
            format = std::move(other.format);

            other.data = nullptr;
        }
        return *this;
    }
};

/**
 * @brief Frame processor function type
 *
 * Takes a FrameData and returns processed FrameData
 */
using PipelineProcessor = std::function<FrameData(const FrameData&)>;

/**
 * @brief Frame output target
 */
class OutputTarget {
public:
    virtual ~OutputTarget() = default;
    virtual void output(const FrameData& frame) = 0;
};

/**
 * @brief Vision pipeline for processing frames from multiple sources
 *
 * The VisionPipeline manages:
 * - Multiple vision device sources (cameras, LiDARs, etc.)
 * - Frame processing chain
 * - Output targets (display, recording, network streaming)
 * - Pipeline lifecycle (start/stop/shutdown)
 */
class VisionPipeline {
public:
    VisionPipeline();
    ~VisionPipeline();

    // Disable copy
    VisionPipeline(const VisionPipeline&) = delete;
    VisionPipeline& operator=(const VisionPipeline&) = delete;

    // Enable move
    VisionPipeline(VisionPipeline&& other) noexcept;
    VisionPipeline& operator=(VisionPipeline&& other) noexcept;

    /**
     * @brief Add a vision device as a source
     * @param device Shared pointer to IVisionDevice
     */
    void addSource(std::shared_ptr<roboclaw::plugins::IVisionDevice> device);

    /**
     * @brief Remove a source from the pipeline
     * @param device Shared pointer to IVisionDevice to remove
     */
    void removeSource(std::shared_ptr<roboclaw::plugins::IVisionDevice> device);

    /**
     * @brief Get number of sources
     * @return Source count
     */
    size_t getSourceCount() const;

    /**
     * @brief Add a frame processor to the pipeline
     * @param processor Processor function
     */
    void addProcessor(PipelineProcessor processor);

    /**
     * @brief Get number of processors
     * @return Processor count
     */
    size_t getProcessorCount() const;

    /**
     * @brief Add an output target
     * @param target Shared pointer to OutputTarget
     */
    void addOutput(std::shared_ptr<OutputTarget> target);

    /**
     * @brief Start the pipeline
     */
    void start();

    /**
     * @brief Stop the pipeline
     */
    void stop();

    /**
     * @brief Shutdown the pipeline and cleanup resources
     */
    void shutdown();

    /**
     * @brief Check if pipeline is running
     * @return true if running
     */
    bool isRunning() const;

    /**
     * @brief Capture a single frame from the first available source
     * @return Captured frame data
     */
    FrameData captureFrame();

    /**
     * @brief Set pipeline mode
     * @param mode Pipeline mode
     */
    void setMode(PipelineMode mode);

    /**
     * @brief Get current pipeline mode
     * @return Current mode
     */
    PipelineMode getMode() const;

private:
    /**
     * @brief Process frame through all processors
     * @param frame Input frame
     * @return Processed frame
     */
    FrameData processFrame(const FrameData& frame);

    /**
     * @brief Send frame to all output targets
     * @param frame Frame to output
     */
    void outputFrame(const FrameData& frame);

    // Source devices
    std::vector<std::shared_ptr<roboclaw::plugins::IVisionDevice>> sources_;

    // Frame processors
    std::vector<PipelineProcessor> processors_;

    // Output targets
    std::vector<std::shared_ptr<OutputTarget>> outputs_;

    // Pipeline state
    std::atomic<bool> running_;
    PipelineMode mode_;

    // Mutex for thread safety
    mutable std::mutex sources_mutex_;
    mutable std::mutex processors_mutex_;
    mutable std::mutex outputs_mutex_;
};

} // namespace roboclaw::vision
