# RoboClaw Embedded Robotics Architecture Design

**Date**: 2025-02-22
**Status**: Design Validated
**Author**: Claude (with user validation)

## Overview

RoboClaw is evolving from a general-purpose AI agent into an embedded robotics development platform targeting Raspberry Pi, Jetson Nano, and similar Linux-based embedded systems. The system provides natural language control of robot hardware, embedded development assistance, and integration with ROS.

## Design Goals

1. **Hardware Abstraction**: Modular interfaces for motors, sensors, and communication
2. **Cross-Platform**: Develop on Mac, deploy to Linux embedded devices
3. **Open Source**: Community-driven testing and contribution
4. **No Hardware Required**: Docker-based development and simulation support

## Architecture

### Three-Layer Design

```
┌─────────────────────────────────────────────────────────┐
│                    Application Layer                    │
│  Robot Control Skills │ Embedded Dev Skills │ ROS Bridge │
├─────────────────────────────────────────────────────────┤
│              Hardware Abstraction Layer (HAL)            │
│   Motor Controllers │ Sensors │ Communication │ Config   │
├─────────────────────────────────────────────────────────┤
│                      Core Layer                          │
│   AI Engine │ Task Parser │ Code Generator │ Session    │
└─────────────────────────────────────────────────────────┘
```

### Core Layer

**Purpose**: AI-driven task understanding and code generation

**Components**:
- **AI Engine**: Extends existing LLM integration with hardware-specific prompts
- **Task Parser**: Converts natural language to hardware operations
- **Code Generator**: Generates C/C++ code for embedded targets
- **Session Manager**: Persists multi-turn conversations

**Key Interfaces**:
```cpp
class ITaskParser {
    virtual Task parseTask(const string& input) = 0;
    virtual bool validateHardwareAccess(const Task& task) = 0;
};

class ICodeGenerator {
    virtual string generateSTM32(const Task& task) = 0;
    virtual string generateArduino(const Task& task) = 0;
    virtual string generateRaspPi(const Task& task) = 0;
};
```

### Hardware Abstraction Layer (HAL)

**Purpose**: Unified interface for diverse hardware components

**Core Interfaces**:

```cpp
class IMotorController {
public:
    virtual bool initialize(const json& config) = 0;
    virtual void setSpeed(int channel, int speed) = 0;
    virtual void setDirection(int channel, bool forward) = 0;
    virtual void stop() = 0;
    virtual ~IMotorController() = default;
};

class ISensor {
public:
    virtual bool initialize(const json& config) = 0;
    virtual json readData() = 0;
    virtual bool isAvailable() = 0;
    virtual string getSensorType() = 0;
    virtual ~ISensor() = default;
};

class IComm {
public:
    virtual bool open(const string& port, int baudrate) = 0;
    virtual bool write(const vector<uint8_t>& data) = 0;
    virtual vector<uint8_t> read(int timeout_ms) = 0;
    virtual void close() = 0;
    virtual ~IComm() = default;
};
```

**Supported Hardware**:
- **Motor Controllers**: RoboClaw, Sabertooth, L298N, PWM drivers
- **Sensors**: MPU6050 (IMU), RPLIDAR, HC-SR04, encoders
- **Communication**: Serial, I2C, SPI, CAN bus

**Hardware Configuration** (TOML):
```toml
[hardware.motor_left]
type = "roboclaw"
port = "/dev/ttyUSB0"
address = 128
max_speed = 255

[hardware.imu]
type = "mpu6050"
bus = "i2c"
address = 0x68

[hardware.lidar]
type = "rplidar_a1"
port = "/dev/ttyUSB1"
```

### Application Layer

**Purpose**: High-level skills for robot control and embedded development

#### Robot Control Skills

| Skill | Description | Parameters |
|-------|-------------|------------|
| `robot_forward` | Move robot forward | `speed` (0-100), `duration` (seconds) |
| `robot_backward` | Move robot backward | `speed` (0-100), `duration` (seconds) |
| `robot_turn` | Turn robot by angle | `angle` (degrees), `speed` (0-100) |
| `robot_stop` | Emergency stop | - |
| `sensor_read_imu` | Read IMU data | - |
| `sensor_read_lidar` | Get obstacle distances | - |
| `pid_tune` | Adjust PID parameters | `axis` (string), `kp`, `ki`, `kd` |

#### Embedded Development Skills

| Skill | Description | Parameters |
|-------|-------------|------------|
| `stm32_gpio_init` | Initialize GPIO | `port` (A-H), `pin` (0-15), `mode` |
| `stm32_uart_config` | Configure UART | `uart_num`, `baudrate`, `parity` |
| `arduino_generate` | Generate Arduino sketch | `description` |
| `flash_via_serial` | Flash firmware | `port`, `firmware_file` |
| `cubemx_generate` | Generate CubeMX project | `mcu_model`, `peripherals` |

#### ROS Bridge Skills

| Skill | Description | Parameters |
|-------|-------------|------------|
| `ros_publish` | Publish to topic | `topic_name`, `message_type`, `data` |
| `ros_subscribe` | Subscribe to topic | `topic_name`, `callback` |
| `ros_service_call` | Call ROS service | `service_name`, `request` |

## Development Strategy

### Docker-Based Development

**Rationale**: Enable Mac development for Linux embedded targets

```dockerfile
FROM ubuntu:22.04

# Install ROS 2 Humble
RUN apt update && apt install -y \
    ros-humble-ros-base \
    build-essential \
    cmake \
    python3-colcon-common-extensions

# Install serial tools
RUN apt install -y python3-serial minicom

# Mount project source
WORKSPACE /workspace
```

### Modular Implementation Phases

1. **Phase 1**: Core Layer extensions
   - Hardware-aware task parsing
   - STM32/Arduino code generation templates

2. **Phase 2**: HAL implementation
   - Serial communication wrapper
   - RoboClaw motor driver
   - MPU6050 sensor driver

3. **Phase 3**: Application skills
   - Basic motion control
   - IMU reading
   - STM32 GPIO init

4. **Phase 4**: ROS integration
   - ROS 2 publisher/subscriber bridges
   - TF2 integration for coordinate transforms

5. **Phase 5**: Testing and simulation
   - Gazebo simulation models
   - Hardware-in-the-loop testing

## Error Handling

```cpp
class HardwareException : public std::runtime_error {
public:
    HardwareException(const string& component, const string& details)
        : std::runtime_error("[" + component + "] " + details) {}
};

class CommException : public HardwareException {
public:
    CommException(const string& port, const string& details)
        : HardwareException("Comm:" + port, details) {}
};
```

## Testing Strategy

1. **Unit Tests**: Mock hardware interfaces
2. **Integration Tests**: Hardware-in-the-loop with real devices
3. **Simulation Tests**: Gazebo with ROS 2
4. **Community Testing**: Open source beta releases

## Dependencies

### Current
- cpr (HTTP)
- nlohmann/json (JSON parsing)

### New Dependencies
- libserialport (serial communication)
- I2Cdevlib (I2C sensor access)
- ROS 2 Humble (robotics middleware)
- Gazebo (simulation)

## File Structure Addition

```
RoboClaw/
├── src/
│   ├── hal/                 # Hardware Abstraction Layer
│   │   ├── motor_controller.h/cpp
│   │   ├── sensor.h/cpp
│   │   └── comm.h/cpp
│   ├── hal/drivers/         # Concrete driver implementations
│   │   ├── roboclaw_driver.cpp
│   │   ├── mpu6050_driver.cpp
│   │   └── rplidar_driver.cpp
│   ├── skills/robot/        # Robot control skills
│   │   └── motion_skill.cpp
│   ├── skills/embedded/     # Embedded dev skills
│   │   └── stm32_skill.cpp
│   └── ros/                 # ROS integration
│       └── ros_bridge.cpp
├── configs/
│   └── hardware.toml        # Hardware configuration
├── docker/
│   ├── Dockerfile
│   └── docker-compose.yml
└── tests/hardware/          # Hardware tests
```

## Open Source Strategy

1. **License**: MIT License
2. **Repository**: GitHub with issue templates
3. **Documentation**: Wiki for hardware setup guides
4. **Community**: Discord/Discord for real-time support
5. **CI/CD**: GitHub Actions with QEMU for ARM builds

## Success Criteria

1. Robot controlled via natural language commands
2. STM32/Arduino code generated and flashed
3. ROS integration working with Gazebo
4. Community members successfully deploying on hardware
