// tests/vision/test_vision_pipeline.cpp
#include <catch2/catch.hpp>
#include "vision/vision_pipeline.h"
#include "vision/frame_processor.h"
#include "plugins/interfaces/ivision_device.h"

using namespace roboclaw::vision;
using namespace roboclaw::plugins;

// Mock frame processor for testing
class MockFrameProcessor : public FrameProcessor {
public:
    MockFrameProcessor() : callCount(0), modifyWidth(false), newWidth(0) {}

    FrameData process(const FrameData& frame) override {
        callCount++;
        FrameData result = frame;
        if (modifyWidth) {
            result.width = newWidth;
        }
        return result;
    }

    void reset() override {
        callCount = 0;
        modifyWidth = false;
    }

    std::string getName() const override {
        return "MockFrameProcessor";
    }

    int callCount;
    bool modifyWidth;
    int newWidth;
};

// Mock vision device for testing
class MockVisionDevice : public IVisionDevice {
public:
    MockVisionDevice() : open_(false), streaming_(false) {}

    std::string getName() const override { return "mock_camera"; }
    std::string getVersion() const override { return "1.0.0"; }

    bool initialize(const nlohmann::json& config) override {
        if (!config.contains("width") || !config.contains("height")) {
            throw std::runtime_error("Invalid config: missing width/height");
        }
        width_ = config["width"].get<int>();
        height_ = config["height"].get<int>();
        return true;
    }

    void shutdown() override {
        if (isOpen()) closeDevice();
    }

    bool openDevice(const std::string& config) override {
        open_ = true;
        return true;
    }

    void closeDevice() override {
        open_ = false;
        streaming_ = false;
    }

    FrameData captureFrame() override {
        FrameData frame;
        frame.width = width_;
        frame.height = height_;
        frame.channels = 3;
        frame.stride = width_ * 3;
        frame.format = "RGB8";
        frame.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        frame.data = new uint8_t[width_ * height_ * 3];
        // Fill with gray pattern
        memset(frame.data, 128, width_ * height_ * 3);
        return frame;
    }

    void setParameter(const std::string& key, const nlohmann::json& value) override {
        params_[key] = value;
    }

    nlohmann::json getParameter(const std::string& key) override {
        if (params_.contains(key)) {
            return params_[key];
        }
        return nullptr;
    }

    nlohmann::json getDeviceCapabilities() override {
        return {
            {"streams", {"color", "depth"}},
            {"width_range", {640, 1920}},
            {"height_range", {480, 1080}},
            {"fps_range", {1, 60}}
        };
    }

    void startStream(int fps) override {
        streaming_ = true;
        fps_ = fps;
    }

    void stopStream() override {
        streaming_ = false;
    }

    void registerFrameCallback(std::function<void(const FrameData&)> callback) override {
        callback_ = callback;
    }

    bool isOpen() const override { return open_; }
    bool isStreaming() const override { return streaming_; }

private:
    bool open_;
    bool streaming_;
    int width_ = 640;
    int height_ = 480;
    int fps_ = 30;
    nlohmann::json params_;
    std::function<void(const FrameData&)> callback_;
};

TEST_CASE("Vision pipeline construction and basic state", "[pipeline]") {
    VisionPipeline pipeline;

    SECTION("New pipeline is not running") {
        REQUIRE_FALSE(pipeline.isRunning());
    }

    SECTION("New pipeline has no sources") {
        REQUIRE(pipeline.getSourceCount() == 0);
    }
}

TEST_CASE("Add source to pipeline", "[pipeline]") {
    VisionPipeline pipeline;
    auto source = std::make_shared<MockVisionDevice>();

    SECTION("Add single source") {
        pipeline.addSource(source);
        REQUIRE(pipeline.getSourceCount() == 1);
    }

    SECTION("Add multiple sources") {
        pipeline.addSource(source);
        pipeline.addSource(std::make_shared<MockVisionDevice>());
        REQUIRE(pipeline.getSourceCount() == 2);
    }
}

TEST_CASE("Start and stop pipeline", "[pipeline]") {
    VisionPipeline pipeline;
    auto source = std::make_shared<MockVisionDevice>();
    nlohmann::json config = {{"width", 640}, {"height", 480}};

    SECTION("Start pipeline") {
        source->initialize(config);
        pipeline.addSource(source);
        pipeline.start();

        REQUIRE(pipeline.isRunning());
    }

    SECTION("Stop pipeline") {
        source->initialize(config);
        pipeline.addSource(source);
        pipeline.start();
        pipeline.stop();

        REQUIRE_FALSE(pipeline.isRunning());
    }
}

TEST_CASE("Capture frame from pipeline", "[pipeline]") {
    VisionPipeline pipeline;
    auto source = std::make_shared<MockVisionDevice>();
    nlohmann::json config = {{"width", 640}, {"height", 480}};

    source->initialize(config);
    pipeline.addSource(source);
    pipeline.start();

    SECTION("Capture returns valid frame") {
        auto frame = pipeline.captureFrame();
        REQUIRE(frame.width == 640);
        REQUIRE(frame.height == 480);
        REQUIRE(frame.channels == 3);
        REQUIRE(frame.format == "RGB8");
        REQUIRE(frame.data != nullptr);
    }

    SECTION("Multiple captures return frames") {
        auto frame1 = pipeline.captureFrame();
        auto frame2 = pipeline.captureFrame();

        REQUIRE(frame1.width == frame2.width);
        REQUIRE(frame1.height == frame2.height);
    }
}

TEST_CASE("Pipeline modes", "[pipeline]") {
    VisionPipeline pipeline;

    SECTION("Default mode is REALTIME") {
        REQUIRE(pipeline.getPipelineMode() == PipelineMode::REALTIME);
    }

    SECTION("Set mode to DETECTION") {
        pipeline.setPipelineMode(PipelineMode::DETECTION);
        REQUIRE(pipeline.getPipelineMode() == PipelineMode::DETECTION);
    }

    SECTION("Set mode to RECORDING") {
        pipeline.setPipelineMode(PipelineMode::RECORDING);
        REQUIRE(pipeline.getPipelineMode() == PipelineMode::RECORDING);
    }
}

TEST_CASE("Frame processor management", "[pipeline]") {
    VisionPipeline pipeline;
    auto source = std::make_shared<MockVisionDevice>();
    nlohmann::json config = {{"width", 640}, {"height", 480}};

    source->initialize(config);
    pipeline.addSource(source);

    SECTION("Add processor") {
        auto processor = std::make_shared<MockFrameProcessor>();
        pipeline.addProcessor(processor);
        REQUIRE(pipeline.getProcessorCount() == 1);
    }

    SECTION("Remove processor") {
        auto processor = std::make_shared<MockFrameProcessor>();
        pipeline.addProcessor(processor);
        REQUIRE(pipeline.getProcessorCount() == 1);

        pipeline.removeProcessor(processor);
        REQUIRE(pipeline.getProcessorCount() == 0);
    }

    SECTION("Processors are called during capture") {
        auto processor = std::make_shared<MockFrameProcessor>();
        processor->modifyWidth = true;
        processor->newWidth = 800;

        pipeline.addProcessor(processor);
        pipeline.start();

        auto frame = pipeline.captureFrame();
        REQUIRE(processor->callCount == 1);
        REQUIRE(frame.width == 800); // Should be modified by processor
    }

    SECTION("Multiple processors are called in order") {
        auto processor1 = std::make_shared<MockFrameProcessor>();
        auto processor2 = std::make_shared<MockFrameProcessor>();

        pipeline.addProcessor(processor1);
        pipeline.addProcessor(processor2);
        pipeline.start();

        auto frame = pipeline.captureFrame();
        REQUIRE(processor1->callCount == 1);
        REQUIRE(processor2->callCount == 1);
    }
}

TEST_CASE("Remove source from pipeline", "[pipeline]") {
    VisionPipeline pipeline;
    auto source = std::make_shared<MockVisionDevice>();
    nlohmann::json config = {{"width", 640}, {"height", 480}};

    source->initialize(config);
    pipeline.addSource(source);
    REQUIRE(pipeline.getSourceCount() == 1);

    SECTION("Remove existing source") {
        pipeline.removeSource(source);
        REQUIRE(pipeline.getSourceCount() == 0);
    }
}

TEST_CASE("Pipeline cleanup on shutdown", "[pipeline]") {
    VisionPipeline pipeline;
    auto source = std::make_shared<MockVisionDevice>();
    nlohmann::json config = {{"width", 640}, {"height", 480}};

    source->initialize(config);
    pipeline.addSource(source);
    pipeline.start();

    REQUIRE(pipeline.isRunning());

    SECTION("Shutdown stops pipeline and cleans up") {
        pipeline.shutdown();
        REQUIRE_FALSE(pipeline.isRunning());
        REQUIRE(pipeline.getSourceCount() == 0);
    }
}
