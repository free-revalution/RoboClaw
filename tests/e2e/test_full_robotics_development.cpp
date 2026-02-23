// tests/e2e/test_full_robotics_development.cpp
// End-to-End Test: Complete Robotics Development Workflow

#include <gtest/gtest.h>
#include "../../src/vision/vision_pipeline.h"
#include "../../src/embedded/workflow_controller.h"
#include "../../src/simulation/simulation_controller.h"
#include "../../src/plugins/interfaces/ivision_device.h"
#include "../../src/plugins/interfaces/iembedded_platform.h"

using namespace roboclaw::vision;
using namespace roboclaw::embedded;
using namespace roboclaw::simulation;
using namespace roboclaw::plugins;

// Mock implementations for testing
class MockVisionDevice : public IVisionDevice {
public:
    std::string getName() const override { return "mock_camera"; }
    std::string getVersion() const override { return "1.0.0"; }
    bool initialize(const nlohmann::json& config) override { return true; }
    void shutdown() override {}
    bool openDevice(const std::string& config) override { open_ = true; return true; }
    void closeDevice() override { open_ = false; }
    FrameData captureFrame() override {
        FrameData frame;
        frame.width = 640;
        frame.height = 480;
        frame.channels = 3;
        frame.format = "RGB8";
        return frame;
    }
    void setParameter(const std::string& key, const nlohmann::json& value) override {}
    nlohmann::json getParameter(const std::string& key) override { return nullptr; }
    nlohmann::json getDeviceCapabilities() override {
        return {{"streams", {"color"}}, {"fps", 30}};
    }
    void startStream(int fps) override { streaming_ = true; }
    void stopStream() override { streaming_ = false; }
    void registerFrameCallback(std::function<void(const FrameData&)> callback) override {}
    bool isOpen() const override { return open_; }
    bool isStreaming() const override { return streaming_; }
private:
    bool open_ = false;
    bool streaming_ = false;
};

class MockEmbeddedPlatform : public IEmbeddedPlatform {
public:
    std::string getName() const override { return "mock_stm32"; }
    std::string getVersion() const override { return "1.0.0"; }
    bool initialize(const nlohmann::json& config) override { return true; }
    void shutdown() override {}

    bool configureProject(const ProjectConfig& config) override { return configured_; }
    bool generateCode(const DriverSpec& spec) override { return true; }
    bool buildProject() override { return true; }

    nlohmann::json optimizeParameters(const std::string& method,
                                      const nlohmann::json& current_params,
                                      const std::vector<TestResult>& test_data) override {
        nlohmann::json result;
        result["kp"] = 2.5;
        result["ki"] = 0.8;
        result["kd"] = 0.3;
        return result;
    }

    std::vector<std::string> getOptimizationMethods() const override {
        return {"zigler_nichols", "genetic_algorithm"};
    }

    std::vector<ProgrammerInfo> detectProgrammers() override {
        return {{"stlink_0", "ST-Link V2", "/dev/ttyUSB0", true}};
    }

    bool flashFirmware(const std::string& firmware_path, const std::string& programmer_id) override {
        return true;
    }

    bool verifyFirmware(const std::string& firmware_path) override { return true; }

    std::vector<ProgrammerInfo> scanConnectedHardware() override {
        return detectProgrammers();
    }

private:
    bool configured_ = true;
};

class MockSimulationTool : public ISimulationTool {
public:
    std::string getName() const override { return "mock_gazebo"; }
    std::string getVersion() const override { return "1.0.0"; }
    bool initialize(const nlohmann::json& config) override { return true; }
    void shutdown() override {}

    bool loadModel(const std::string& model_path) override {
        model_loaded_ = true;
        return true;
    }
    void unloadModel() override { model_loaded_ = false; }

    SimulationResult runTest(const TestScenario& scenario) override {
        SimulationResult result;
        result.success = true;
        result.duration = scenario.duration;
        result.metrics = {{"position_reached", true}, {"final_error", 0.01}};
        return result;
    }

    nlohmann::json extractMetrics() override {
        return {{"position_x", 5.0}, {"position_y", 3.0}};
    }

    nlohmann::json getMetric(const std::string& metric_name) override {
        if (metric_name == "position_x") return 5.0;
        if (metric_name == "position_y") return 3.0;
        return nullptr;
    }

    bool syncParametersToHardware(const nlohmann::json& params) override { return true; }

    bool startSimulation() override { return true; }
    void stopSimulation() override {}
    void pauseSimulation() override {}
    void resumeSimulation() override {}
    void resetSimulation() override {}
    bool isRunning() const override { return false; }
    bool isModelLoaded() const override { return model_loaded_; }
    void setTimeStep(double dt) override {}
    double getSimulationTime() const override { return 0.0; }

private:
    bool model_loaded_ = false;
};

// ============================================================================
// E2E Test: Complete Robotics Development Cycle
// ============================================================================

TEST(E2ERoboticsDevelopment, FullWorkflowTest) {
    // Step 1: Vision System Setup
    VisionPipeline vision;
    auto camera = std::make_shared<MockVisionDevice>();
    camera->initialize(nlohmann::json{});
    vision.addSource(camera);
    vision.start();

    ASSERT_TRUE(vision.isRunning());

    // Step 2: Capture and process frame
    auto frame = vision.captureFrame();
    ASSERT_EQ(frame.width, 640);
    ASSERT_EQ(frame.height, 480);

    // Step 3: Configure embedded project
    WorkflowController embedded;
    auto platform = std::make_shared<MockEmbeddedPlatform>();
    platform->initialize(nlohmann::json{});
    embedded.setPlatform(platform);

    PeripheralConfig peripherals;
    peripherals.peripherals = {
        {"uart1", {{"baudrate", 115200}, {"tx", "PA9"}, {"rx", "PA10"}}},
        {"spi1", {{"mosi", "PA7"}, {"miso", "PA6"}, {"sck", "PA5"}}}
    };

    ASSERT_TRUE(embedded.configureCubeMX(MCUModel::STM32_F4, peripherals));

    // Step 4: Generate driver code
    DriverSpec motor_spec{"motor_control", {{"type", "pid"}, {"channels", 2}}};
    auto generated_files = embedded.generateDriverCode(motor_spec);
    ASSERT_FALSE(generated_files.empty());

    // Step 5: Optimize parameters
    TestConfig test_config{"step", 5.0, 1.0, 1.0, {}};
    OptimizationConstraints constraints;
    constraints.max_overshoot = 5.0;

    auto optimized_params = embedded.optimizeParameters("genetic_algorithm", test_config, constraints);
    ASSERT_TRUE(optimized_params.contains("kp"));
    ASSERT_TRUE(optimized_params.contains("ki"));
    ASSERT_TRUE(optimized_params.contains("kd"));

    // Step 6: Generate simulation model
    SimulationController simulation;
    auto sim_tool = std::make_shared<MockSimulationTool>();
    sim_tool->initialize(nlohmann::json{});
    simulation.setSimulationTool(sim_tool);

    HardwareConfig hw_config;
    hw_config.robot_name = "test_robot";
    hw_config.drive_wheels = {"left_wheel", "right_wheel"};
    hw_config.sensors = {"lidar", "camera"};

    auto urdf_path = simulation.generateURDF(hw_config);
    ASSERT_FALSE(urdf_path.empty());

    // Step 7: Load and run simulation
    ASSERT_TRUE(simulation.loadSimulation(urdf_path));

    TestScenario nav_scenario;
    nav_scenario.name = "navigation_test";
    nav_scenario.duration = 3.0;
    nav_scenario.metrics_to_collect = {"position_reached", "final_error"};

    auto sim_result = simulation.runTestScenario(nav_scenario);
    ASSERT_TRUE(sim_result.success);

    // Step 8: Extract parameters from simulation
    auto sim_params = simulation.extractOptimizedParameters();
    ASSERT_TRUE(sim_params.contains("speed_pid"));

    // Step 9: Sim-to-Real transfer
    Sim2RealTransfer sim2real;
    CalibrationMatrix calibration;
    calibration.position_scale = 0.95;

    auto hw_params = sim2real.mapSimulationToHardware(sim_params, calibration);
    ASSERT_TRUE(hw_params.contains("speed_pid"));

    // Step 10: Safety verification
    auto safety_check = sim2real.verifySafeDeployment(hw_params);
    ASSERT_TRUE(safety_check.passed);

    // Step 11: Progressive deployment
    auto stages = sim2real.getDefaultDeploymentStages();
    bool deployment_success = sim2real.progressiveDeployment(hw_params, stages);
    ASSERT_TRUE(deployment_success);

    // Cleanup
    vision.shutdown();
}

TEST(E2ERoboticsDevelopment, VisionToSimulationIntegration) {
    // Test integration between vision and simulation modules
    VisionPipeline vision;
    auto camera = std::make_shared<MockVisionDevice>();
    camera->initialize(nlohmann::json{});
    vision.addSource(camera);
    vision.start();

    SimulationController simulation;
    auto sim_tool = std::make_shared<MockSimulationTool>();
    sim_tool->initialize(nlohmann::json{});
    simulation.setSimulationTool(sim_tool);

    // Vision data informs simulation
    auto frame = vision.captureFrame();
    ASSERT_EQ(frame.format, "RGB8");

    // Could use frame data to configure simulation
    HardwareConfig hw_config;
    hw_config.robot_name = "vision_guided_robot";
    hw_config.sensors = {"camera"};

    auto urdf_path = simulation.generateURDF(hw_config);
    ASSERT_FALSE(urdf_path.empty());

    vision.shutdown();
}

TEST(E2ERoboticsDevelopment, EmbeddedToSimulationLoop) {
    // Test parameter optimization loop between embedded and simulation
    WorkflowController embedded;
    auto platform = std::make_shared<MockEmbeddedPlatform>();
    platform->initialize(nlohmann::json{});
    embedded.setPlatform(platform);

    SimulationController simulation;
    auto sim_tool = std::make_shared<MockSimulationTool>();
    sim_tool->initialize(nlohmann::json{});
    simulation.setSimulationTool(sim_tool);

    // Initial parameters
    nlohmann::json initial_params = {{"kp", 1.0}, {"ki", 0.1}, {"kd", 0.05}};

    // Run in simulation
    TestScenario test_scenario;
    test_scenario.name = "pid_tuning";
    test_scenario.duration = 2.0;
    test_scenario.metrics_to_collect = {"overshoot", "settling_time"};

    auto sim_result = simulation.runTestScenario(test_scenario);
    ASSERT_TRUE(sim_result.success);

    // Optimize based on simulation results
    std::vector<TestResult> test_data = {
        {0.5, initial_params, 0}
    };

    auto optimized_params = platform->optimizeParameters("zigler_nichols", initial_params, test_data);
    ASSERT_TRUE(optimized_params.contains("kp"));

    // Verify parameters are safe for hardware
    Sim2RealTransfer sim2real;
    auto safety_check = sim2real.verifySafeDeployment(optimized_params);
    ASSERT_TRUE(safety_check.passed);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
