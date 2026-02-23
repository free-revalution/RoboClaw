// src/vision/frame_processor.h
#pragma once

#include <memory>
#include <functional>
#include <string>
#include "../plugins/interfaces/ivision_device.h"

namespace roboclaw::vision {

/**
 * @brief Base class for frame processors
 *
 * FrameProcessor provides a way to process frames in a pipeline.
 * Derived classes can implement specific processing algorithms
 * like preprocessing, feature extraction, AI inference, etc.
 */
class FrameProcessor {
public:
    virtual ~FrameProcessor() = default;

    /**
     * @brief Process a single frame
     * @param frame Input frame data
     * @return Processed frame data
     */
    virtual roboclaw::plugins::FrameData process(const roboclaw::plugins::FrameData& frame) = 0;

    /**
     * @brief Reset the processor state
     *
     * Called when the pipeline is restarted or the processor
     * needs to clear its internal state.
     */
    virtual void reset() {}

    /**
     * @brief Get processor name for debugging
     * @return Processor name
     */
    virtual std::string getName() const { return "FrameProcessor"; }
};

/**
 * @brief Convenience processor that wraps a lambda function
 */
class LambdaProcessor : public FrameProcessor {
public:
    using ProcessorFunc = std::function<roboclaw::plugins::FrameData(const roboclaw::plugins::FrameData&)>;

    explicit LambdaProcessor(ProcessorFunc fn)
        : func_(std::move(fn)) {}

    roboclaw::plugins::FrameData process(const roboclaw::plugins::FrameData& frame) override {
        return func_(frame);
    }

    std::string getName() const override { return "LambdaProcessor"; }

private:
    ProcessorFunc func_;
};

} // namespace roboclaw::vision
