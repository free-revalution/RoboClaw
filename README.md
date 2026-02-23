<div align="center">

# RoboClaw

### 🤖 AI-Powered Robotics Development Agent with Natural Language Interface

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++20](https://img.shes.io/badge/C++-20-00599C.svg)](https://en.cppreference.com/w/cpp/20)
[![CMake](https://img.shields.io/badge/CMake-3.20%2B-blue.svg)](https://cmake.org/)
[![Platform](https://img.shields.io/badge/platform-macOS%20%7C%20Linux%20%7C%20Windows-lightgrey.svg)](#installation)
[![GitHub Stars](https://img.shields.io/github/stars/free-revalution/RoboClaw?style=social)](https://github.com/free-revalution/RoboClaw)

**Your Intelligent AI Partner for Robotics and Software Development**

[English](#english) | [简体中文](#简体中文)

---

<a id="english"></a>
</div>

## What is RoboClaw?

**RoboClaw** is a cutting-edge AI Agent framework that revolutionizes how developers interact with their codebase and robotics hardware. By combining natural language understanding with powerful automation tools, RoboClaw acts as your intelligent development companion.

### 🌟 Key Highlights

- **🧠 Natural Language Interface** - Control everything with plain English or Chinese commands
- **🔌 Extensible Plugin System** - Modular architecture for vision, embedded, and simulation tools
- **🤖 Robotics-First Design** - Built-in support for LiDAR, cameras, motor controllers, and sensors
- **⚡ Lightning Fast** - C++20 powered with multithreading and zero-copy optimizations
- **🌍 Cross-Platform** - Works seamlessly on macOS, Linux, and Windows

---

## Why RoboClaw?

### 🎯 Unlike Traditional Tools

| Traditional IDE/Tools | RoboClaw Agent |
|----------------------|----------------|
| Manual code editing | Natural language commands |
| Separate tools for each task | Unified AI-powered interface |
| Hardware-specific SDKs | Generic plugin abstraction |
| Complex build processes | One-command automation |
| Static documentation | Interactive AI assistance |

### 💡 Powerful Capabilities

**For Software Developers:**
- Read, write, and edit code files conversationally
- Execute shell commands safely with AI oversight
- Automate browser interactions for testing
- Discover and coordinate with other AI agents

**For Robotics Engineers:**
- Control hardware with natural language
- Integrate vision sensors (LiDAR, depth cameras)
- Automate embedded development workflows
- Simulate and test in Gazebo/ROS 2

---

## Features

### 🛠️ Core AI Tools

| Tool | Description |
|------|-------------|
| **Read** | Intelligently read and summarize files |
| **Write** | Generate code from natural language descriptions |
| **Edit** | Make precise code changes with context awareness |
| **Bash** | Execute commands with safety validation |
| **Serial** | Communicate with embedded hardware |
| **Browser** | Automate web interactions and testing |
| **Agent** | Coordinate with other AI assistants |

### 🔬 Vision & Perception Module

> **NEW in v0.4.0** - Complete vision pipeline for robotics applications

- **Multi-Source Support**: RealSense cameras, RPLIDAR, industrial cameras
- **Real-Time Processing**: SLAM, obstacle detection, navigation
- **Data Recording**: Capture sensor data for offline analysis
- **Plugin Architecture**: Easy integration of new vision devices

```
# Example: Start LiDAR scanning
>>> /connect rplidar
>>> Start scanning at 10Hz
>>> Show obstacle distances
Front: 1.2m | Right: 2.3m | Back: 0.8m | Left: 0.5m
```

### 🔌 Plugin System

RoboClaw's modular plugin architecture allows endless extensions:

- **Vision Plugins**: Support for any camera or LiDAR
- **Embedded Plugins**: STM32, ESP32, Arduino platforms
- **Simulation Plugins**: Gazebo, Webots integration

### 🤖 Hardware Control

**Supported Hardware:**
- Motor Controllers: RoboClaw, Sabertooth, L298N
- Sensors: IMU (MPU6050), LiDAR, Ultrasonic, Encoders
- Communication: UART, I2C, SPI

### 🔗 Social Platform Integration

Control your robots remotely through:
- Telegram Bot API
- DingTalk / Feishu (Enterprise)

---

## Quick Start

### One-Line Installation

```bash
curl -sSL https://raw.githubusercontent.com/free-revalution/RoboClaw/main/install.sh | bash
```

### Manual Installation

```bash
git clone https://github.com/free-revalution/RoboClaw.git
cd RoboClaw
./scripts/install.sh
```

### Build from Source

```bash
# Install dependencies (macOS)
brew install cmake ninja nlohmann-json

# Configure and build
cmake --preset=release
cmake --build build --config Release

# Run RoboClaw
./build/roboclaw
```

---

## Usage Examples

### Example 1: Natural Language Code Editing

```bash
roboclaw

>>> Read the MotionSkill class and add a rotateInPlace function
[RoboClaw reads the file and generates the function]

>>> Add error handling for the serial communication
[RoboClaw adds try-catch blocks with appropriate logging]
```

### Example 2: Hardware Control

```bash
roboclaw

>>> Connect to the motor controller on /dev/ttyUSB0
Connected to RoboClaw motor controller

>>> Move forward at 60% speed for 3 seconds
[Motors running... Done]

>>> Stop immediately
[Emergency stop activated]
```

### Example 3: Vision System

```bash
roboclaw

>>> Start the RealSense camera
RealSense D435 connected at 640x480@30fps

>>> Enable obstacle detection and alert when closer than 0.5m
Obstacle detection enabled - Threshold: 0.5m

>>> Record point cloud data to maps/room_scan.pcd
Recording... [10000 frames captured]
```

---

## Architecture

RoboClaw's layered architecture ensures modularity and extensibility:

```
┌─────────────────────────────────────────────────────────────┐
│                    Natural Language Interface                │
│                  "Start SLAM" | "Tune PID" | "Flash"         │
├─────────────────────────────────────────────────────────────┤
│                      Application Layer                       │
│  ┌─────────────┐ ┌──────────────┐ ┌─────────────────────┐ │
│  │    Vision   │ │   Embedded   │ │   Simulation &      │ │
│  │    Module   │ │   Dev Auto   │ │   Verification      │ │
│  └─────────────┘ └──────────────┘ └─────────────────────┘ │
├─────────────────────────────────────────────────────────────┤
│                       Plugin Layer                           │
│  Vision Devices | MCU Platforms | Sim Tools | Optimizers   │
├─────────────────────────────────────────────────────────────┤
│                        HAL Layer                             │
│  Device Abstraction | Communication | Data Pipeline         │
├─────────────────────────────────────────────────────────────┤
│                         Core Layer                           │
│  AI Engine | Task Parser | Code Generator | Session Manager │
└─────────────────────────────────────────────────────────────┘
```

---

## Project Structure

```
RoboClaw/
├── src/
│   ├── agent/              # Core AI Agent engine
│   ├── vision/             # Vision perception pipeline
│   ├── plugins/            # Plugin system & interfaces
│   ├── tools/              # AI-powered tools
│   ├── hal/                # Hardware Abstraction Layer
│   ├── skills/             # Robot control skills
│   └── llm/                # LLM provider interface
├── plugins/
│   ├── vision/             # Vision device plugins
│   │   ├── realsense2/
│   │   └── rplidar/
│   ├── embedded/           # Embedded platform plugins
│   └── simulation/         # Simulation tool plugins
├── docs/                   # Comprehensive documentation
└── tests/                  # Unit, integration, E2E tests
```

---

## Documentation

- [Quick Start Guide](docs/quickstart.md)
- [Vision Module Guide](docs/vision-guide.md)
- [Hardware Integration](docs/hardware-integration.md)
- [Plugin Development](docs/plugin-development.md)
- [API Reference](docs/api-reference.md)

---

## Contributing

We welcome contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for details.

**Areas needing help:**
- Additional vision sensor plugins
- More embedded platform support
- Simulation tool integrations
- Documentation improvements
- Bug fixes and performance optimizations

---

## Roadmap

- [x] **v0.1** - Core AI Agent framework
- [x] **v0.2** - Browser automation & Agent discovery
- [x] **v0.3** - Hardware control & Social platform integration
- [x] **v0.4** - Vision perception module & Plugin system
- [ ] **v0.5** - Embedded development automation
- [ ] **v0.6** - ROS/Gazebo simulation integration
- [ ] **v0.7** - Complete robotics development platform

---

## License

MIT License - see [LICENSE](LICENSE) for details.

---

## Acknowledgments

- [OpenClaw](https://github.com/OpenClaw) - Inspiration for visual browser control
- [nlohmann/json](https://github.com/nlohmann/json) - Excellent JSON library
- [CPR](https://github.com/libcpr/cpr) - Modern C++ HTTP library
- All contributors and supporters

---

<div align="center">

**Built with ❤️ by the Robotics and AI Community**

[⭐ Star](https://github.com/free-revalution/RoboClaw) &nbsp;&nbsp;
[🍴 Fork](https://github.com/free-revalution/RoboClaw/fork) &nbsp;&nbsp;
[📖 Docs](https://github.com/free-revalution/RoboClaw/wiki) &nbsp;&nbsp;
[🐛 Issues](https://github.com/free-revalution/RoboClaw/issues)

**Join our community and help build the future of robotics development!**

</div>

---

<a id="简体中文"></a>

<div align="center">

# RoboClaw

### 🤖 基于自然语言接口的AI驱动机器人开发Agent

[![许可协议: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/C++-20-00599C.svg)](https://en.cppreference.com/w/cpp/20)
[![CMake](https://img.shields.io/badge/CMake-3.20%2B-blue.svg)](https://cmake.org/)
[![平台支持](https://img.shields.io/badge/platform-macOS%20%7C%20Linux%20%7C%20Windows-lightgrey.svg)](#installation)
[![GitHub Stars](https://img.shields.io/github/stars/free-revalution/RoboClaw?style=social)](https://github.com/free-revalution/RoboClaw)

**您的智能AI伙伴，用于机器人开发和软件开发**

</div>

## RoboClaw是什么？

**RoboClaw**是一个革命性的AI Agent框架，它彻底改变了开发者与代码库和机器人硬件的交互方式。通过将自然语言理解与强大的自动化工具相结合，RoboClaw成为您智能开发的伙伴。

### 🌟 核心亮点

- **🧠 自然语言接口** - 使用纯中文或英文命令控制一切
- **🔌 可扩展插件系统** - 支持视觉、嵌入式和仿真工具的模块化架构
- **🤖 机器人优先设计** - 内置LiDAR、摄像头、电机控制器和传感器支持
- **⚡ 极速性能** - C++20驱动，多线程和零拷贝优化
- **🌍 跨平台支持** - 在macOS、Linux和Windows上无缝工作

---

## 为什么选择RoboClaw？

### 🎯 与传统工具的区别

| 传统IDE/工具 | RoboClaw Agent |
|-------------|---------------|
| 手动代码编辑 | 自然语言命令 |
| 每个任务单独的工具 | 统一的AI驱动接口 |
| 硬件特定的SDK | 通用插件抽象 |
| 复杂的构建流程 | 一键自动化 |
| 静态文档 | 交互式AI辅助 |

### 💡 强大功能

**面向软件开发者：**
- 通过对话方式读取、编写和编辑代码文件
- 在AI监督下安全执行shell命令
- 自动化浏览器交互进行测试
- 发现并协调其他AI助手

**面向机器人工程师：**
- 使用自然语言控制硬件
- 集成视觉传感器（LiDAR、深度摄像头）
- 自动化嵌入式开发工作流
- 在Gazebo/ROS 2中仿真和测试

---

## 核心功能

### 🛠️ AI核心工具

| 工具 | 描述 |
|------|------|
| **Read** | 智能读取和总结文件 |
| **Write** | 根据自然语言描述生成代码 |
| **Edit** | 带上下文感知的精确代码更改 |
| **Bash** | 带安全验证的命令执行 |
| **Serial** | 与嵌入式硬件通信 |
| **Browser** | 自动化Web交互和测试 |
| **Agent** | 协调其他AI助手 |

### 🔬 视觉感知模块

> **v0.4.0新增** - 用于机器人应用的完整视觉管道

- **多源支持**：RealSense摄像头、RPLIDAR、工业摄像头
- **实时处理**：SLAM、障碍物检测、导航
- **数据录制**：捕获传感器数据用于离线分析
- **插件架构**：轻松集成新的视觉设备

```
# 示例：启动LiDAR扫描
>>> /connect rplidar
>>> 开始10Hz扫描
>>> 显示障碍物距离
前方: 1.2m | 右侧: 2.3m | 后方: 0.8m | 左侧: 0.5m
```

---

## 快速开始

### 一键安装

```bash
curl -sSL https://raw.githubusercontent.com/free-revalution/RoboClaw/main/install.sh | bash
```

### 从源码构建

```bash
# 安装依赖 (macOS)
brew install cmake ninja nlohmann-json

# 配置并构建
cmake --preset=release
cmake --build build --config Release

# 运行RoboClaw
./build/roboclaw
```

---

## 使用示例

### 示例1：自然语言代码编辑

```bash
roboclaw

>>> 读取MotionSkill类并添加rotateInPlace函数
[RoboClaw读取文件并生成函数]

>>> 为串口通信添加错误处理
[RoboClaw添加try-catch块和适当的日志]
```

### 示例2：硬件控制

```bash
roboclaw

>>> 连接/dev/ttyUSB0上的电机控制器
已连接到RoboClaw电机控制器

>>> 以60%速度前进3秒
[电机运行中... 完成]

>>> 立即停止
[紧急停止已激活]
```

---

## 许可证

MIT License - 详见 [LICENSE](LICENSE)

---

<div align="center">

**用 ❤️ 构建 | 机器人和AI社区**

[⭐ Star](https://github.com/free-revalution/RoboClaw) &nbsp;&nbsp;
[🍴 Fork](https://github.com/free-revalution/RoboClaw/fork) &nbsp;&nbsp;
[📖 文档](https://github.com/free-revalution/RoboClaw/wiki)

**加入我们的社区，共同构建机器人开发的未来！**

</div>
