// src/vision/vision_pipeline.cpp
#include "vision_pipeline.h"
#include <cstring>
#include <algorithm>

namespace roboclaw::vision {

VisionPipeline::VisionPipeline()
    : running_(false)
    , mode_(PipelineMode::REALTIME) {
}

VisionPipeline::~VisionPipeline() {
    shutdown();
}

VisionPipeline::VisionPipeline(VisionPipeline&& other) noexcept
    : sources_(std::move(other.sources_))
    , processors_(std::move(other.processors_))
    , outputs_(std::move(other.outputs_))
    , running_(other.running_.load())
    , mode_(other.mode_) {
}

VisionPipeline& VisionPipeline::operator=(VisionPipeline&& other) noexcept {
    if (this != &other) {
        std::lock(sources_mutex_, processors_mutex_, outputs_mutex_,
                  other.sources_mutex_, other.processors_mutex_, other.outputs_mutex_);

        sources_ = std::move(other.sources_);
        processors_ = std::move(other.processors_);
        outputs_ = std::move(other.outputs_);
        running_ = other.running_.load();
        mode_ = other.mode_;

        other.sources_mutex_.unlock();
        other.processors_mutex_.unlock();
        other.outputs_mutex_.unlock();
        sources_mutex_.unlock();
        processors_mutex_.unlock();
        outputs_mutex_.unlock();
    }
    return *this;
}

void VisionPipeline::addSource(std::shared_ptr<roboclaw::plugins::IVisionDevice> device) {
    if (!device) {
        return;
    }

    std::lock_guard<std::mutex> lock(sources_mutex_);
    sources_.push_back(device);
}

void VisionPipeline::removeSource(std::shared_ptr<roboclaw::plugins::IVisionDevice> device) {
    std::lock_guard<std::mutex> lock(sources_mutex_);

    auto it = std::find(sources_.begin(), sources_.end(), device);
    if (it != sources_.end()) {
        sources_.erase(it);
    }
}

size_t VisionPipeline::getSourceCount() const {
    std::lock_guard<std::mutex> lock(sources_mutex_);
    return sources_.size();
}

void VisionPipeline::addProcessor(std::shared_ptr<FrameProcessor> processor) {
    if (!processor) {
        return;
    }

    std::lock_guard<std::mutex> lock(processors_mutex_);
    processors_.push_back(processor);
}

void VisionPipeline::removeProcessor(std::shared_ptr<FrameProcessor> processor) {
    std::lock_guard<std::mutex> lock(processors_mutex_);

    auto it = std::find(processors_.begin(), processors_.end(), processor);
    if (it != processors_.end()) {
        processors_.erase(it);
    }
}

size_t VisionPipeline::getProcessorCount() const {
    std::lock_guard<std::mutex> lock(processors_mutex_);
    return processors_.size();
}

void VisionPipeline::addOutput(std::shared_ptr<OutputTarget> target) {
    if (!target) {
        return;
    }

    std::lock_guard<std::mutex> lock(outputs_mutex_);
    outputs_.push_back(target);
}

void VisionPipeline::start() {
    running_ = true;
}

void VisionPipeline::stop() {
    running_ = false;
}

void VisionPipeline::shutdown() {
    stop();

    std::lock_guard<std::mutex> s_lock(sources_mutex_);
    std::lock_guard<std::mutex> p_lock(processors_mutex_);
    std::lock_guard<std::mutex> o_lock(outputs_mutex_);

    // Close all devices
    for (auto& source : sources_) {
        if (source && source->isOpen()) {
            source->closeDevice();
        }
    }

    sources_.clear();
    processors_.clear();
    outputs_.clear();
}

bool VisionPipeline::isRunning() const {
    return running_.load();
}

FrameData VisionPipeline::captureFrame() {
    std::lock_guard<std::mutex> lock(sources_mutex_);

    if (sources_.empty()) {
        // Return empty frame
        return FrameData{};
    }

    // Get frame from first available source
    for (auto& source : sources_) {
        if (source && source->isOpen()) {
            // Convert IVisionDevice::FrameData to our FrameData
            auto deviceFrame = source->captureFrame();

            FrameData frame;
            frame.data = deviceFrame.data;
            frame.width = deviceFrame.width;
            frame.height = deviceFrame.height;
            frame.channels = deviceFrame.channels;
            frame.stride = deviceFrame.stride;
            frame.timestamp = deviceFrame.timestamp;
            frame.format = deviceFrame.format;

            // Process through processors
            FrameData processedFrame = processFrame(frame);

            // Don't delete deviceFrame.data here - it's owned by deviceFrame's destructor

            return processedFrame;
        }
    }

    return FrameData{};
}

void VisionPipeline::setPipelineMode(PipelineMode mode) {
    mode_ = mode;
}

PipelineMode VisionPipeline::getPipelineMode() const {
    return mode_;
}

FrameData VisionPipeline::processFrame(const FrameData& frame) {
    FrameData result = frame;

    std::lock_guard<std::mutex> lock(processors_mutex_);

    for (auto& processor : processors_) {
        if (processor) {
            result = processor(result);
        }
    }

    return result;
}

void VisionPipeline::outputFrame(const FrameData& frame) {
    std::lock_guard<std::mutex> lock(outputs_mutex_);

    for (auto& output : outputs_) {
        if (output) {
            output->output(frame);
        }
    }
}

} // namespace roboclaw::vision
