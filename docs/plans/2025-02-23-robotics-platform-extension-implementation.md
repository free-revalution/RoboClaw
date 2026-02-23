# Robotics Platform Extension Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Extend RoboClaw into a comprehensive robotics development platform with vision perception, embedded development automation, and ROS/Gazebo simulation integration.

**Architecture:** Layered plugin architecture with three major modules built on a core plugin system. All features accessible through natural language dialogue.

**Tech Stack:** C++20, CMake, nlohmann/json, plugin system (dlopen), ROS 2 Humble, Gazebo 11, OpenCV, PCL

---

## Phase 1: Core Plugin System (Week 1-2)

### Task 1.1: Plugin Base Interface

**Files:**
- Create: `src/plugins/plugin.h`
- Test: `tests/plugins/test_plugin_interface.cpp`

**Step 1: Write the failing test**

```cpp
// tests/plugins/test_plugin_interface.cpp
#include <catch2/catch.hpp>
#include "plugins/plugin.h"

using namespace roboclaw::plugins;

TEST_CASE("Plugin interface provides name and version", "[plugin]") {
    MockPlugin plugin;
    REQUIRE(plugin.getName() == "mock");
    REQUIRE(plugin.getVersion() == "1.0.0");
}

TEST_CASE("Plugin initialize with valid config", "[plugin]") {
    MockPlugin plugin;
    nlohmann::json config = {{"key", "value"}};
    REQUIRE(plugin.initialize(config));
}

TEST_CASE("Plugin initialize with invalid config throws", "[plugin]") {
    MockPlugin plugin;
    nlohmann::json config;
    REQUIRE_THROWS_AS(plugin.initialize(config), std::runtime_error);
}
```

**Step 2: Run test to verify it fails**

Run: `cd build && ctest --output-on-failure -R test_plugin_interface`
Expected: FAIL - "MockPlugin not defined"

**Step 3: Write minimal implementation**

```cpp
// src/plugins/plugin.h
#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace roboclaw::plugins {

class IPlugin {
public:
    virtual ~IPlugin() = default;
    virtual std::string getName() const = 0;
    virtual std::string getVersion() const = 0;
    virtual bool initialize(const nlohmann::json& config) = 0;
    virtual void shutdown() = 0;
};

// Mock for testing
class MockPlugin : public IPlugin {
public:
    std::string getName() const override { return "mock"; }
    std::string getVersion() const override { return "1.0.0"; }
    bool initialize(const nlohmann::json& config) override {
        if (!config.contains("key")) throw std::runtime_error("Invalid config");
        return true;
    }
    void shutdown() override {}
};

} // namespace roboclaw::plugins
```

**Step 4: Run test to verify it passes**

Run: `cd build && ctest --output-on-failure -R test_plugin_interface`
Expected: PASS (3/3 tests)

**Step 5: Commit**

```bash
git add src/plugins/plugin.h tests/plugins/test_plugin_interface.cpp
git commit -m "feat(plugin): add base plugin interface with mock

- Define IPlugin interface with getName, getVersion, initialize, shutdown
- Add MockPlugin for testing
- Write tests for interface and config validation"
```

---

### Task 1.2: Plugin Registry

**Files:**
- Create: `src/plugins/plugin_registry.h`
- Create: `src/plugins/plugin_registry.cpp`
- Test: `tests/plugins/test_plugin_registry.cpp`

**Step 1: Write the failing test**

```cpp
// tests/plugins/test_plugin_registry.cpp
#include <catch2/catch.hpp>
#include "plugins/plugin_registry.h"
#include "plugins/plugin.h"

using namespace roboclaw::plugins;

TEST_CASE("Register and retrieve plugin", "[registry]") {
    PluginRegistry<IPlugin> registry;
    auto plugin = std::make_shared<MockPlugin>();

    registry.registerPlugin("mock", plugin);
    auto retrieved = registry.getPlugin("mock");

    REQUIRE(retrieved->getName() == "mock");
}

TEST_CASE("Get non-existent plugin returns null", "[registry]") {
    PluginRegistry<IPlugin> registry;
    REQUIRE(registry.getPlugin("nonexistent") == nullptr);
}

TEST_CASE("List all registered plugins", "[registry]") {
    PluginRegistry<IPlugin> registry;
    registry.registerPlugin("mock1", std::make_shared<MockPlugin>());
    registry.registerPlugin("mock2", std::make_shared<MockPlugin>());

    auto plugins = registry.listPlugins();
    REQUIRE(plugins.size() == 2);
    REQUIRE(plugins[0] == "mock1");
    REQUIRE(plugins[1] == "mock2");
}
```

**Step 2: Run test to verify it fails**

Run: `cd build && ctest --output-on-failure -R test_plugin_registry`
Expected: FAIL - "PluginRegistry not defined"

**Step 3: Write minimal implementation**

```cpp
// src/plugins/plugin_registry.h
#pragma once
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace roboclaw::plugins {

template<typename T>
class PluginRegistry {
public:
    bool registerPlugin(const std::string& id, std::shared_ptr<T> plugin) {
        plugins_[id] = plugin;
        return true;
    }

    std::shared_ptr<T> getPlugin(const std::string& id) const {
        auto it = plugins_.find(id);
        return (it != plugins_.end()) ? it->second : nullptr;
    }

    std::vector<std::string> listPlugins() const {
        std::vector<std::string> ids;
        for (const auto& [id, _] : plugins_) {
            ids.push_back(id);
        }
        return ids;
    }

private:
    std::unordered_map<std::string, std::shared_ptr<T>> plugins_;
};

} // namespace roboclaw::plugins
```

**Step 4: Run test to verify it passes**

Run: `cd build && ctest --output-on-failure -R test_plugin_registry`
Expected: PASS (3/3 tests)

**Step 5: Commit**

```bash
git add src/plugins/plugin_registry.h tests/plugins/test_plugin_registry.cpp
git commit -m "feat(plugin): add plugin registry with basic operations

- Implement PluginRegistry template class
- Support register, get, and list operations
- Add comprehensive tests for registry functionality"
```

---

### Task 1.3: Plugin Manager with Dynamic Loading

**Files:**
- Create: `src/plugins/plugin_manager.h`
- Create: `src/plugins/plugin_manager.cpp`
- Test: `tests/plugins/test_plugin_manager.cpp`

**Step 1: Write the failing test**

```cpp
// tests/plugins/test_plugin_manager.cpp
#include <catch2/catch.hpp>
#include "plugins/plugin_manager.h"

TEST_CASE("Load plugins from directory", "[manager]") {
    PluginManager manager;
    size_t loaded = manager.loadPluginsFromDirectory("tests/fixtures/plugins");
    REQUIRE(loaded > 0);
}

TEST_CASE("Get plugin by ID and type", "[manager]") {
    PluginManager manager;
    manager.loadPluginsFromDirectory("tests/fixtures/plugins");

    auto plugin = manager.getPlugin<IVisionDevice>("mock_camera");
    REQUIRE(plugin != nullptr);
    REQUIRE(plugin->getName() == "mock_camera");
}

TEST_CASE("Get plugin with wrong type returns null", "[manager]") {
    PluginManager manager;
    manager.loadPluginsFromDirectory("tests/fixtures/plugins");

    auto plugin = manager.getPlugin<IEmbeddedPlatform>("mock_camera");
    REQUIRE(plugin == nullptr);
}
```

**Step 2: Run test to verify it fails**

Run: `cd build && ctest --output-on-failure -R test_plugin_manager`
Expected: FAIL - "PluginManager not defined"

**Step 3: Write minimal implementation**

```cpp
// src/plugins/plugin_manager.h
#pragma once
#include "plugin_registry.h"
#include "plugin.h"
#include "interfaces/ivision_device.h"
#include "interfaces/iembedded_platform.h"
#include "interfaces/isimulation_tool.h"
#include <dlfcn.h>
#include <filesystem>

namespace roboclaw::plugins {

class PluginManager {
public:
    size_t loadPluginsFromDirectory(const std::string& directory) {
        size_t count = 0;
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.path().extension() == ".so") {
                if (loadPlugin(entry.path())) count++;
            }
        }
        return count;
    }

    template<typename T>
    std::shared_ptr<T> getPlugin(const std::string& id) {
        return registries_.getRegistry<T>()->getPlugin(id);
    }

private:
    bool loadPlugin(const std::filesystem::path& path) {
        void* handle = dlopen(path.c_str(), RTLD_LAZY);
        if (!handle) return false;

        using CreateFunc = T* (*)();
        auto create = (CreateFunc) dlsym(handle, "create");

        if (create) {
            std::shared_ptr<T> plugin(create());
            registries_.getRegistry<T>()->registerPlugin(plugin->getName(), plugin);
            return true;
        }
        return false;
    }

    struct Registries {
        PluginRegistry<IVisionDevice> vision;
        PluginRegistry<IEmbeddedPlatform> embedded;
        PluginRegistry<ISimulationTool> simulation;

        template<typename T>
        PluginRegistry<T>* getRegistry();
    };

    Registries registries_;
};

} // namespace roboclaw::plugins
```

**Step 4: Create placeholder interfaces**

```cpp
// src/plugins/interfaces/ivision_device.h
#pragma once
#include "plugin.h"
namespace roboclaw::plugins {
class IVisionDevice : public IPlugin {};
}

// src/plugins/interfaces/iembedded_platform.h
#pragma once
#include "plugin.h"
namespace roboclaw::plugins {
class IEmbeddedPlatform : public IPlugin {};
}

// src/plugins/interfaces/isimulation_tool.h
#pragma once
#include "plugin.h"
namespace roboclaw::plugins {
class ISimulationTool : public IPlugin {};
}
```

**Step 5: Run test to verify it passes**

Run: `cd build && ctest --output-on-failure -R test_plugin_manager`
Expected: PASS (3/3 tests)

**Step 6: Commit**

```bash
git add src/plugins/plugin_manager.h src/plugins/plugin_manager.cpp
git add src/plugins/interfaces/
git add tests/plugins/test_plugin_manager.cpp
git commit -m "feat(plugin): add plugin manager with dynamic loading

- Implement PluginManager with dlopen support
- Add plugin interface definitions for three module types
- Support type-safe plugin retrieval"
```

---

## Phase 2: Vision Perception Module (Week 3-5)

### Task 2.1: Vision Data Pipeline

**Files:**
- Create: `src/vision/vision_pipeline.h`
- Create: `src/vision/vision_pipeline.cpp`
- Test: `tests/vision/test_vision_pipeline.cpp`

**Step 1: Write the failing test**

```cpp
// tests/vision/test_vision_pipeline.cpp
#include <catch2/catch.hpp>
#include "vision/vision_pipeline.h"
#include "plugins/interfaces/ivision_device.h"

using namespace roboclaw::vision;

TEST_CASE("Add source and start pipeline", "[pipeline]") {
    VisionPipeline pipeline;
    auto source = std::make_shared<MockVisionDevice>();

    pipeline.addSource(source);
    pipeline.start();

    REQUIRE(pipeline.isRunning());
}

TEST_CASE("Capture frame from pipeline", "[pipeline]") {
    VisionPipeline pipeline;
    pipeline.addSource(std::make_shared<MockVisionDevice>());
    pipeline.start();

    auto frame = pipeline.captureFrame();
    REQUIRE(frame.width == 640);
    REQUIRE(frame.height == 480);
}
```

**Step 2: Run test to verify it fails**

Run: `cd build && ctest --output-on-failure -R test_vision_pipeline`
Expected: FAIL - "VisionPipeline not defined"

**Step 3: Write minimal implementation**

```cpp
// src/vision/vision_pipeline.h
#pragma once
#include <memory>
#include <vector>
#include <functional>
#include "plugins/interfaces/ivision_device.h"

namespace roboclaw::vision {

enum class PipelineMode { REALTIME, DETECTION, RECORDING };

struct FrameData {
    int width;
    int height;
    std::vector<uint8_t> data;
};

class VisionPipeline {
public:
    void addSource(std::shared_ptr<IVisionDevice> device) {
        sources_.push_back(device);
    }

    void start() { running_ = true; }
    void stop() { running_ = false; }
    bool isRunning() const { return running_; }

    FrameData captureFrame() {
        FrameData frame{640, 480, {}};
        frame.data.resize(640 * 480 * 3, 128);
        return frame;
    }

private:
    std::vector<std::shared_ptr<IVisionDevice>> sources_;
    bool running_ = false;
};

} // namespace roboclaw::vision
```

**Step 4: Run test to verify it passes**

Run: `cd build && ctest --output-on-failure -R test_vision_pipeline`
Expected: PASS (2/2 tests)

**Step 5: Commit**

```bash
git add src/vision/vision_pipeline.h src/vision/vision_pipeline.cpp
git add tests/vision/test_vision_pipeline.cpp
git commit -m "feat(vision): add basic vision pipeline

- Implement VisionPipeline with source management
- Support frame capture with mock data
- Add tests for pipeline lifecycle"
```

---

### Task 2.2: RealSense2 Plugin

**Files:**
- Create: `plugins/vision/realsense2/realsense2_plugin.cpp`
- Create: `plugins/vision/realsense2/realsense2_plugin.h`
- Test: `tests/vision/test_realsense2_plugin.cpp`

**Step 1: Write the failing test**

```cpp
// tests/vision/test_realsense2_plugin.cpp
#include <catch2/catch.hpp>
#include "plugins/interfaces/ivision_device.h"

TEST_CASE("RealSense plugin loads and initializes", "[realsense2]") {
    auto plugin = createRealSense2Plugin();

    nlohmann::json config = {{"width", 640}, {"height", 480}};
    REQUIRE(plugin->initialize(config));

    auto capabilities = plugin->getDeviceCapabilities();
    REQUIRE(capabilities["streams"].size() > 0);
}

TEST_CASE("RealSense capture frame returns valid data", "[realsense2]") {
    auto plugin = createRealSense2Plugin();
    plugin->initialize(nlohmann::json{});

    auto frame = plugin->captureFrame();
    REQUIRE(frame.width > 0);
    REQUIRE(frame.height > 0);
}
```

**Step 2: Run test to verify it fails**

Run: `cd build && ctest --output-on-failure -R test_realsense2_plugin`
Expected: FAIL - "createRealSense2Plugin not defined"

**Step 3: Write minimal implementation**

```cpp
// plugins/vision/realsense2/realsense2_plugin.h
#pragma once
#include "../../src/plugins/interfaces/ivision_device.h"

namespace roboclaw::plugins::vision {

class RealSense2Plugin : public IVisionDevice {
public:
    std::string getName() const override { return "realsense2"; }
    std::string getVersion() const override { return "1.0.0"; }

    bool initialize(const nlohmann::json& config) override {
        // TODO: Initialize RealSense SDK
        return true;
    }

    void shutdown() override {}

    FrameData captureFrame() override {
        // TODO: Capture from RealSense
        return {640, 480, {}};
    }

    void setParameter(const std::string& key, const nlohmann::json& value) override {}
    nlohmann::json getDeviceCapabilities() override {
        return {{"streams", {"color", "depth", "infrared"}}};
    }
};

extern "C" {
    IVisionDevice* create() {
        return new RealSense2Plugin();
    }
}

} // namespace roboclaw::plugins::vision
```

**Step 4: Run test to verify it passes**

Run: `cd build && ctest --output-on-failure -R test_realsense2_plugin`
Expected: PASS (2/2 tests)

**Step 5: Commit**

```bash
git add plugins/vision/realsense2/
git add tests/vision/test_realsense2_plugin.cpp
git commit -m "feat(vision): add RealSense2 plugin skeleton

- Implement basic RealSense2 plugin structure
- Add initialization and capability query
- Mock frame capture (SDK integration TODO)"
```

---

### Task 2.3: RPLIDAR Plugin

**Files:**
- Create: `plugins/vision/rplidar/rplidar_plugin.cpp`
- Create: `plugins/vision/rplidar/rplidar_plugin.h`
- Test: `tests/vision/test_rplidar_plugin.cpp`

**Step 1: Write the failing test**

```cpp
// tests/vision/test_rplidar_plugin.cpp
#include <catch2/catch.hpp>
#include "plugins/interfaces/ivision_device.h"

TEST_CASE("RPLIDAR plugin scans and returns points", "[rplidar]") {
    auto plugin = createRPLIDARPlugin();

    nlohmann::json config = {{"port", "/dev/ttyUSB0"}, {"baudrate", 115200}};
    REQUIRE(plugin->initialize(config));

    auto scan = plugin->getScanData();
    REQUIRE(scan.size() == 360);
}
```

**Step 2-5:** Similar pattern to RealSense2 plugin (TDD + commit)

---

## Phase 3: Embedded Development Automation (Week 6-9)

### Task 3.1: Embedded Workflow Controller

**Files:**
- Create: `src/embedded/workflow_controller.h`
- Create: `src/embedded/workflow_controller.cpp`
- Test: `tests/embedded/test_workflow_controller.cpp`

**Step 1: Write the failing test**

```cpp
// tests/embedded/test_workflow_controller.cpp
#include <catch2/catch.hpp>
#include "embedded/workflow_controller.h"

TEST_CASE("Configure STM32 project", "[workflow]") {
    EmbeddedWorkflowController controller;

    MCUModel mcu{"STM32F407"};
    PeripheralConfig peripherals;
    peripherals.add("UART1", {{"baudrate", 115200}});

    REQUIRE(controller.configureCubeMX(mcu, peripherals));
}

TEST_CASE("Generate driver code", "[workflow]") {
    EmbeddedWorkflowController controller;

    DriverSpec spec{"motor_controller", "PWM"};
    auto files = controller.generateDriverCode(spec);

    REQUIRE(files.size() == 2);  // .h and .cpp
}
```

**Step 2-5:** TDD + implementation + commit

---

### Task 3.2: Parameter Optimizer Framework

**Files:**
- Create: `src/embedded/optimizers/i_parameter_optimizer.h`
- Create: `src/embedded/optimizers/ziegler_nichols_optimizer.cpp`
- Test: `tests/embedded/test_optimizers.cpp`

**Step 1: Write the failing test**

```cpp
// tests/embedded/test_optimizers.cpp
#include <catch2/catch.hpp>
#include "embedded/optimizers/ziegler_nichols_optimizer.h"

TEST_CASE("Ziegler-Nichols optimizes PID", "[optimizer]") {
    ZieglerNicholsOptimizer optimizer;

    PlantModel plant{"first_order", 1.0, 0.5};
    nlohmann::json current_params = {{"kp", 1.0}, {"ki", 0.0}, {"kd", 0.0}};
    OptimizationConstraints constraints;
    constraints.maxOvershoot = 0.1;

    auto result = optimizer.optimize(plant, current_params, constraints);

    REQUIRE(result.parameters["kp"] > 0);
    REQUIRE(result.parameters["ki"] > 0);
}
```

**Step 2-5:** TDD + implementation + commit

---

### Task 3.3: Programmer Detector

**Files:**
- Create: `src/embedded/programmer_detector.h`
- Create: `src/embedded/programmer_detector.cpp`
- Test: `tests/embedded/test_programmer_detector.cpp`

**Step 1: Write the failing test**

```cpp
// tests/embedded/test_programmer_detector.cpp
#include <catch2/catch.hpp>
#include "embedded/programmer_detector.h"

TEST_CASE("Detect ST-Link programmer", "[programmer]") {
    ProgrammerDetector detector;
    auto programmers = detector.scanProgrammers();

    bool found_stlink = false;
    for (const auto& p : programmers) {
        if (p.name == "ST-Link V2") found_stlink = true;
    }

    REQUIRE(found_stlink);
}

TEST_CASE("Flash firmware via ST-Link", "[programmer]") {
    ProgrammerDetector detector;

    FlashResult result = detector.flashFirmware(
        "stlink_0",
        "test_firmware.hex",
        {{"verify", true}}
    );

    REQUIRE(result.success);
}
```

**Step 2-5:** TDD + implementation + commit

---

## Phase 4: Simulation & Verification Module (Week 10-12)

### Task 4.1: Simulation Controller

**Files:**
- Create: `src/simulation/simulation_controller.h`
- Create: `src/simulation/simulation_controller.cpp`
- Test: `tests/simulation/test_simulation_controller.cpp`

**Step 1: Write the failing test**

```cpp
// tests/simulation/test_simulation_controller.cpp
#include <catch2/catch.hpp>
#include "simulation/simulation_controller.h"

TEST_CASE("Generate URDF from hardware config", "[simulation]") {
    SimulationController controller;

    HardwareConfig config;
    config.addWheel("left", {{"port", "A1"}});
    config.addWheel("right", {{"port", "A2"}});

    auto urdf = controller.generateURDF(config);

    REQUIRE(urdf.find("robot name") != std::string::npos);
}

TEST_CASE("Run test scenario and get results", "[simulation]") {
    SimulationController controller;
    controller.loadSimulation("models/test_robot.urdf");

    TestScenario scenario;
    scenario.goal = "navigate to (5, 3)";

    auto result = controller.runTestScenario(scenario);

    REQUIRE(result.success);
    REQUIRE(result.metrics["time_to_goal"] > 0);
}
```

**Step 2-5:** TDD + implementation + commit

---

### Task 4.2: Sim-to-Real Transfer

**Files:**
- Create: `src/simulation/sim2real_transfer.h`
- Create: `src/simulation/sim2real_transfer.cpp`
- Test: `tests/simulation/test_sim2real.cpp`

**Step 1: Write the failing test**

```cpp
// tests/simulation/test_sim2real.cpp
#include <catch2/catch.hpp>
#include "simulation/sim2real_transfer.h"

TEST_CASE("Map simulation params to hardware", "[sim2real]") {
    Sim2RealTransfer transfer;

    nlohmann::json sim_params = {{"kp", 2.0}};
    CalibrationMatrix calibration = {{"scale", 0.95}};

    auto hw_params = transfer.mapSimulationToHardware(sim_params, calibration);

    REQUIRE(hw_params["kp"] == 1.9);  // 2.0 * 0.95
}

TEST_CASE("Safety check rejects dangerous params", "[sim2real]") {
    Sim2RealTransfer transfer;

    nlohmann::json unsafe_params = {{"kp", 100.0}};

    auto check = transfer.verifySafeDeployment(unsafe_params);

    REQUIRE_FALSE(check.passed);
}
```

**Step 2-5:** TDD + implementation + commit

---

## Phase 5: Integration & Testing (Week 13-14)

### Task 5.1: End-to-End Integration Test

**Files:**
- Create: `tests/e2e/test_full_robot_development.cpp`

**Step 1: Write the failing test**

```cpp
// tests/e2e/test_full_robot_development.cpp
#include <catch2/catch.hpp>
#include "vision/vision_pipeline.h"
#include "embedded/workflow_controller.h"
#include "simulation/simulation_controller.h"

TEST_CASE("Complete robot development workflow", "[e2e]") {
    // 1. Configure vision
    VisionPipeline vision;
    vision.addSource(loadPlugin("realsense2"));
    vision.start();

    // 2. Generate embedded code
    EmbeddedWorkflowController embedded;
    embedded.configureCubeMX({"STM32F407"}, {});
    embedded.generateDriverCode({"motor", "PID"});

    // 3. Optimize parameters in simulation
    SimulationController sim;
    sim.loadSimulation(generateURDF());
    auto sim_result = sim.runTestScenario({"navigate"});
    auto params = sim.extractOptimizedParameters();

    // 4. Deploy to hardware
    Sim2RealTransfer transfer;
    auto hw_params = transfer.mapSimulationToHardware(params, {});
    REQUIRE(transfer.verifySafeDeployment(hw_params).passed);

    // 5. Flash firmware
    ProgrammerDetector prog;
    REQUIRE(prog.flashFirmware("stlink", "firmware.hex", {}));
}
```

**Step 2-5:** TDD + implementation + commit

---

### Task 5.2: Documentation and Examples

**Files:**
- Create: `docs/vision-guide.md`
- Create: `docs/embedded-dev-guide.md`
- Create: `docs/simulation-guide.md`

**Step 1: Write vision guide**

```markdown
# Vision Perception Guide

## Overview
RoboClaw vision pipeline supports LiDAR, depth cameras, and industrial cameras.

## Usage Examples

### Start LiDAR scan
```
>>> /link connect rplidar
>>> 启动激光雷达扫描
>>> 显示障碍物距离
```

### Record depth camera data
```
>>> 启动深度相机录制
>>> 保存到 maps/room_scan.pcd
```
```

**Step 2-5:** Write remaining guides and commit

---

### Task 5.3: Plugin Development Guide

**Files:**
- Create: `docs/plugin-development-guide.md`

**Step 1: Write plugin guide**

```markdown
# Plugin Development Guide

## Creating a Vision Plugin

1. Implement IVisionDevice interface
2. Compile as shared library (.so)
3. Install to plugins/vision/
4. Test with /plugin list

## Example

See plugins/vision/realsense2/ for reference implementation.
```

**Step 2-5:** Complete guide and commit

---

## Testing Requirements

### Unit Tests
- All core classes > 80% coverage
- All plugins have mock-based tests

### Integration Tests
- Cross-module interaction tests
- Hardware simulation tests

### E2E Tests
- Complete robot development cycle
- Multi-scenario testing

---

## Build Instructions

```bash
# Configure
mkdir build && cd build
cmake ..

# Build
make -j4

# Test
ctest --output-on-failure
```

---

## Dependencies

### Required
- C++20 compiler
- CMake 3.20+
- nlohmann/json 3.11.3+

### Optional (for specific plugins)
- librealsense2 (RealSense cameras)
- rplidar_ros (RPLIDAR)
- ROS 2 Humble (simulation module)
- Gazebo 11 (simulation module)

---

## Success Criteria

- [ ] All tests passing (unit + integration + e2e)
- [ ] At least 2 vision plugins working
- [ ] Embedded workflow functional with STM32
- [ ] Simulation integration with Gazebo
- [ ] Documentation complete
- [ ] Code reviewed and merged to main
