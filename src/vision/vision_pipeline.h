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
 * @brief Frame output target
 */
class OutputTarget {
public:
    virtual ~OutputTarget() = default;
    virtual void output(const roboclaw::plugins::FrameData& frame) = 0;
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
     * @param processor Shared pointer to FrameProcessor
     */
    void addProcessor(std::shared_ptr<FrameProcessor> processor);

    /**
     * @brief Remove a processor from the pipeline
     * @param processor Shared pointer to FrameProcessor to remove
     */
    void removeProcessor(std::shared_ptr<FrameProcessor> processor);

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
    roboclaw::plugins::FrameData captureFrame();

    /**
     * @brief Set pipeline mode
     * @param mode Pipeline mode
     */
    void setPipelineMode(PipelineMode mode);

    /**
     * @brief Get current pipeline mode
     * @return Current mode
     */
    PipelineMode getPipelineMode() const;

private:
    /**
     * @brief Process frame through all processors
     * @param frame Input frame
     * @return Processed frame
     */
    roboclaw::plugins::FrameData processFrame(const roboclaw::plugins::FrameData& frame);

    /**
     * @brief Send frame to all output targets
     * @param frame Frame to output
     */
    void outputFrame(const roboclaw::plugins::FrameData& frame);

    // Source devices
    std::vector<std::shared_ptr<roboclaw::plugins::IVisionDevice>> sources_;

    // Frame processors
    std::vector<std::shared_ptr<FrameProcessor>> processors_;

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
