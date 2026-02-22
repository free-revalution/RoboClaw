# RoboPartner Embedded Robotics Platform - Quick Start

## Overview / 概述

**[English]**

RoboPartner Embedded Robotics Platform extends RoboPartner with hardware control capabilities for robotics development. It provides a hardware abstraction layer (HAL) for controlling motors, reading sensors, and managing communication protocols on embedded Linux systems like Raspberry Pi and Jetson Nano.

**[中文]**

RoboPartner 嵌入式机器人平台扩展了 RoboPartner 的硬件控制能力，为机器人开发提供硬件抽象层（HAL）。它支持在 Raspberry Pi 和 Jetson Nano 等嵌入式 Linux 系统上控制电机、读取传感器和管理通信协议。

---

## Hardware Requirements / 硬件要求

### Supported Platforms / 支持的平台

| Platform | Architecture | Status / 状态 |
|----------|-------------|---------------|
| Raspberry Pi 4 | ARM64 | ✅ Fully Supported |
| Raspberry Pi 3B+ | ARM32 | ✅ Supported |
| Jetson Nano | ARM64 | ✅ Supported |
| Jetson Orin | ARM64 | ✅ Supported |
| BeagleBone Black | ARM32 | 🔄 In Development |

### Supported Hardware / 支持的硬件

#### Motor Controllers / 电机控制器

- **RoboClaw** (2x7A, 2x15A, 2x30A)
- **Sabertooth** (2x5A, 2x12A, 2x25A, 2x32A)
- **L298N** Dual H-Bridge
- **TB6612FNG** Dual Motor Driver
- Custom PWM drivers

#### Sensors / 传感器

- **IMU**: MPU6050, MPU9250, BNO055
- **LiDAR**: RPLIDAR A1/A2, YDLIDAR X4
- **Ultrasonic**: HC-SR04, US-015
- **Encoders**: Quadrature encoders (360+ PPR)
- **Distance**: Sharp IR sensors, VL53L0X ToF

#### Communication / 通信

- **Serial/UART**: RoboClaw, Sabertooth
- **I2C**: MPU6050, BNO055
- **SPI**: Custom sensors
- **CAN**: CAN bus devices (planned)

---

## Installation / 安装

### Quick Install / 快速安装

#### On Raspberry Pi / Jetson Nano

```bash
# Clone repository / 克隆仓库
git clone https://github.com/yourusername/RoboClaw.git
cd RoboClaw

# Install dependencies / 安装依赖
sudo apt update
sudo apt install -y cmake build-essential nlohmann-json3-dev

# Build / 编译
mkdir build && cd build
cmake ..
make -j4

# Install / 安装
sudo make install

# Add user to dialout group for serial access / 将用户添加到 dialout 组以访问串口
sudo usermod -aG dialout $USER
```

#### Cross-Compilation from macOS

```bash
# Install ARM toolchain / 安装 ARM 工具链
brew install arm-none-eabi-gcc cmake

# Configure for ARM cross-compilation / 配置 ARM 交叉编译
cmake -DCMAKE_TOOLCHAIN_FILE=cmake/arm-linux-gnueabihf.cmake -B build

# Build / 编译
cmake --build build --config Release

# Transfer to target device / 传输到目标设备
scp build/robopartner pi@raspberrypi.local:~/
```

---

## Hardware Configuration / 硬件配置

### Step 1: Connect Hardware / 连接硬件

**RoboClaw Connection Example / RoboClaw 连接示例**:

```
Raspberry Pi          RoboClaw Motor Controller
┌─────────────┐       ┌──────────────────┐
│             │       │                  │
│  5V ────────┼───────┤ VIN              │
│  GND ───────┼───────┤ GND              │
│  TX (14) ───┼───────┤ S1 (RX)          │
│  RX (15) ───┼───────┤ S2 (TX)          │
│             │       │                  │
└─────────────┘       └──────────────────┘
```

**MPU6050 I2C Connection / MPU6050 I2C 连接**:

```
Raspberry Pi          MPU6050 IMU
┌─────────────┐       ┌──────────────┐
│             │       │              │
│  3.3V ──────┼───────┤ VCC          │
│  GND ───────┼───────┤ GND          │
│  SDA (2) ───┼───────┤ SDA          │
│  SCL (3) ───┼───────┤ SCL          │
│             │       │              │
└─────────────┘       └──────────────┘
```

### Step 2: Configure Hardware / 配置硬件

```bash
# Copy example configuration / 复制示例配置
mkdir -p ~/.robopartner
cp configs/hardware.json.example ~/.robopartner/hardware.json

# Edit configuration / 编辑配置
nano ~/.robopartner/hardware.json
```

**Configuration Example / 配置示例**:

```json
{
  "motors": {
    "motor_left": {
      "type": "roboclaw",
      "port": "/dev/ttyUSB0",
      "address": 128,
      "channel": 0,
      "max_speed": 255
    },
    "motor_right": {
      "type": "roboclaw",
      "port": "/dev/ttyUSB0",
      "address": 128,
      "channel": 1,
      "max_speed": 255
    }
  },
  "sensors": {
    "imu": {
      "type": "mpu6050",
      "bus": "i2c",
      "address": 104
    },
    "lidar": {
      "type": "rplidar_a1",
      "port": "/dev/ttyUSB1"
    }
  }
}
```

---

## Quick Start / 快速开始

### 1. Verify Hardware Detection / 验证硬件检测

```bash
# List all configured hardware / 列出所有配置的硬件
robopartner hardware list

# Output / 输出:
# Motors / 电机:
#   - motor_left (RoboClaw @ /dev/ttyUSB0:128)
#   - motor_right (RoboClaw @ /dev/ttyUSB0:128)
#
# Sensors / 传感器:
#   - imu (MPU6050 @ I2C 0x68)
#   - lidar (RPLIDAR A1 @ /dev/ttyUSB1)
```

### 2. Test Hardware Connection / 测试硬件连接

```bash
# Test hardware connections / 测试硬件连接
robopartner hardware test

# Output / 输出:
# [OK] motor_left: Connected
# [OK] motor_right: Connected
# [OK] imu: Responding
# [WARN] lidar: Not detected (optional)
```

### 3. Start Robot Control / 启动机器人控制

```bash
# Launch robot control agent / 启动机器人控制 Agent
robopartner agent --launch robot-controller
```

### 4. Interactive Control / 交互式控制

```bash
# Enter interactive mode / 进入交互模式
robopartner

# Natural language commands / 自然语言命令:
>>> 前进 50% 速度 2 秒
>>> 左转 90 度
>>> 读取 IMU 数据
>>> 停止
```

---

## Usage Examples / 使用示例

### Example 1: Basic Motion Control / 基本运动控制

```cpp
#include "skills/robot/motion_skill.h"
#include "hal/drivers/roboclaw_driver.h"

using namespace roboclaw::skills;
using namespace roboclaw::hal::drivers;

// Create motor controller / 创建电机控制器
auto motorController = std::make_shared<RoboClawDriver>();
motorController->initialize({
    {"port", "/dev/ttyUSB0"},
    {"address", 128}
});

// Create motion skill / 创建运动技能
MotionSkill motion(motorController);

// Control robot / 控制机器人
motion.forward(50, 2.0);  // Forward at 50% speed for 2 seconds / 前进 50% 速度 2 秒
motion.turn(90, 50);      // Turn right 90 degrees at 50% speed / 右转 90 度 50% 速度
motion.stop();            // Emergency stop / 紧急停止
```

### Example 2: Sensor Reading / 传感器读取

```cpp
#include "skills/robot/sensor_skill.h"
#include "hal/drivers/mpu6050_driver.h"

using namespace roboclaw::skills;
using namespace roboclaw::hal::drivers;

// Create sensor skill / 创建传感器技能
SensorSkill sensors;

// Register IMU / 注册 IMU
auto imu = std::make_shared<MPU6050Driver>();
imu->initialize({{"address", 104}});
sensors.registerSensor("imu", imu);

// Read sensor data / 读取传感器数据
auto data = sensors.readSensor("imu");
std::cout << "Acceleration: "
          << data["accel"]["x"] << ", "
          << data["accel"]["y"] << ", "
          << data["accel"]["z"] << std::endl;

// Read all sensors / 读取所有传感器
auto allData = sensors.readAll();
std::cout << allData.dump(2) << std::endl;
```

### Example 3: Custom Motor Driver / 自定义电机驱动

```cpp
#include "hal/motor_controller.h"

using namespace roboclaw::hal;

class CustomMotorDriver : public IMotorController {
public:
    bool initialize(const nlohmann::json& config) override {
        // Custom initialization / 自定义初始化
        port_ = config["port"];
        address_ = config["address"];
        return true;
    }

    void setSpeed(int channel, int speed) override {
        // Custom speed control / 自定义速度控制
        sendCommand(channel, SET_SPEED, speed);
    }

    void setDirection(int channel, bool forward) override {
        // Custom direction control / 自定义方向控制
        sendCommand(channel, SET_DIRECTION, forward ? 1 : 0);
    }

    void stop() override {
        sendCommand(0, EMERGENCY_STOP);
        sendCommand(1, EMERGENCY_STOP);
    }

    bool isConnected() const override {
        return checkConnection();
    }

private:
    std::string port_;
    int address_;

    void sendCommand(int channel, int cmd, int value = 0);
    bool checkConnection() const;
};
```

---

## API Reference / API 参考

### HAL Interfaces / HAL 接口

#### IMotorController / 电机控制器接口

```cpp
class IMotorController {
    virtual bool initialize(const json& config) = 0;
    virtual void setSpeed(int channel, int speed) = 0;
    virtual void setDirection(int channel, bool forward) = 0;
    virtual void stop() = 0;
    virtual bool isConnected() const = 0;
};
```

#### ISensor / 传感器接口

```cpp
class ISensor {
    virtual bool initialize(const json& config) = 0;
    virtual json readData() = 0;
    virtual bool isAvailable() = 0;
    virtual std::string getSensorType() = 0;
};
```

#### IComm / 通信接口

```cpp
class IComm {
    virtual bool open(const std::string& port, int baudrate) = 0;
    virtual bool write(const std::vector<uint8_t>& data) = 0;
    virtual std::vector<uint8_t> read(int timeout_ms) = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;
};
```

### Robot Skills / 机器人技能

#### MotionSkill / 运动技能

| Method | Description | Parameters |
|--------|-------------|------------|
| `forward(speed, duration)` | Move forward | speed: 0-100%, duration: seconds |
| `backward(speed, duration)` | Move backward | speed: 0-100%, duration: seconds |
| `turn(angle, speed)` | Turn robot | angle: degrees (+right, -left), speed: 0-100% |
| `stop()` | Emergency stop | - |

#### SensorSkill / 传感器技能

| Method | Description | Returns |
|--------|-------------|---------|
| `registerSensor(name, sensor)` | Register a sensor | - |
| `readSensor(name)` | Read specific sensor | JSON data |
| `readAll()` | Read all sensors | JSON with all data |
| `isAvailable(name)` | Check sensor status | bool |

---

## Troubleshooting / 故障排除

### Serial Port Permission Issues / 串口权限问题

```bash
# Problem / 问题: Permission denied on /dev/ttyUSB0
# Solution / 解决方案:
sudo usermod -aG dialout $USER
# Log out and log back in / 重新登录
```

### I2C Not Detected / I2C 未检测到

```bash
# Enable I2C on Raspberry Pi / 在 Raspberry Pi 上启用 I2C
sudo raspi-config
# Navigate: Interface Options -> I2C -> Enable
# 导航: 接口选项 -> I2C -> 启用

# Verify I2C devices / 验证 I2C 设备
i2cdetect -y 1
```

### Motor Not Responding / 电机不响应

```bash
# Check serial connection / 检查串口连接
ls -l /dev/ttyUSB*

# Test serial communication / 测试串口通信
sudo minicom -D /dev/ttyUSB0 -b 115200

# Verify RoboClaw address / 验证 RoboClaw 地址
# Default address is 128 (0x80) / 默认地址是 128 (0x80)
```

---

## Advanced Topics / 高级主题

### Docker Development / Docker 开发

```dockerfile
FROM ubuntu:22.04

# Install dependencies / 安装依赖
RUN apt update && apt install -y \
    cmake build-essential \
    nlohmann-json3-dev \
    python3-serial minicom

# Mount project / 挂载项目
WORKDIR /workspace
COPY . .

# Build / 编译
RUN mkdir build && cd build && \
    cmake .. && make -j4
```

### ROS 2 Integration / ROS 2 集成

```cpp
#include "ros/ros_bridge.h"

// Create ROS bridge / 创建 ROS 桥接
ros::RosBridge bridge;

// Publish motor commands / 发布电机命令
bridge.publish("/cmd_vel", {
    {"linear", {{"x", 0.5}, {"y", 0}, {"z", 0}}},
    {"angular", {{"x", 0}, {"y", 0}, {"z", 0.3}}}
});

// Subscribe to sensor data / 订阅传感器数据
bridge.subscribe("/imu", [](const json& data) {
    std::cout << "IMU: " << data.dump() << std::endl;
});
```

### Gazebo Simulation / Gazebo 仿真

```xml
<!-- robot_gazebo.launch.py -->
from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='gazebo_ros',
            executable='gzserver',
            arguments=['/path/to/robot.model'],
        ),
        Node(
            package='robopartner',
            executable='robot_controller',
            output='screen',
        ),
    ])
```

---

## Contributing / 贡献

We welcome contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

我们欢迎贡献！请参阅 [CONTRIBUTING.md](CONTRIBUTING.md) 了解指南。

---

## License / 许可证

MIT License - see [LICENSE](LICENSE) for details.

MIT 许可证 - 详情见 [LICENSE](LICENSE)。

---

## Support / 支持

- **Documentation**: [docs/](docs/)
- **Issues**: [GitHub Issues](https://github.com/yourusername/RoboClaw/issues)
- **Discussions**: [GitHub Discussions](https://github.com/yourusername/RoboClaw/discussions)

---

**Made with ❤️ by the RoboPartner Community**

**用 ❤️ 构建 | RoboPartner 社区**
