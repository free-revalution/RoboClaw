<div align="center">

# RoboClaw

### AI Agent Framework with Browser Automation / 带浏览器自动化的 AI Agent 框架

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/C++-20-00599C.svg)](https://en.cppreference.com/w/cpp/20)
[![CMake](https://img.shields.io/badge/CMake-3.20%2B-blue.svg)](https://cmake.org/)
[![Platform](https://img.shields.io/badge/platform-macOS%20%7C%20Linux%20%7C%20Windows-lightgrey.svg)](README.md#installation)

**"Your AI Development Partner" / "您的 AI 开发伙伴"**

</div>

---

## Introduction / 简介

**[English]**

RoboClaw is an advanced AI Agent framework written in modern C++ that combines powerful code understanding with browser automation capabilities. Inspired by OpenClaw's visual interaction model and extending it with agent discovery and management, RoboClaw becomes your ultimate AI development companion:

- **7 Core Tools**: Read, Write, Edit, Bash, Serial, **Browser (NEW)**, **Agent Manager (NEW)**
- **Browser Automation**: Visual browser control similar to OpenClaw - navigate, click, type, screenshot
- **Agent Discovery**: Automatically detect and manage local AI coding assistants (Claude Code, Cursor, Copilot, etc.)
- **Minimal System Prompt**: The shortest possible Agent system prompt
- **Self-Coding**: Agent writes its own code for new features instead of installing plugins
- **Tree-Structured Conversations**: Support branching with bug fixes that don't pollute mainline
- **High Performance**: Multithreaded with thread pool, read-write locks, and atomic operations
- **Production Ready**: Token optimization, session persistence, cross-platform support
- **Multilingual**: English and Simplified Chinese language support

**[中文]**

RoboClaw 是一个用现代 C++ 编写的高级 AI Agent 框架，结合了强大的代码理解和浏览器自动化能力。灵感来源于 OpenClaw 的可视化交互模型，并扩展了 Agent 发现和管理功能，使其成为您的终极 AI 开发伙伴：

- **7 个核心工具**：Read、Write、Edit、Bash、Serial、**Browser（浏览器自动化，新增）**、**Agent Manager（Agent 管理，新增）**
- **浏览器自动化**：类似 OpenClaw 的可视化操作 - 导航、点击、输入、截图
- **Agent 发现**：自动检测和管理本地 AI 编程助手（Claude Code、Cursor、Copilot 等）
- **极简系统提示词**：最短的 Agent 系统提示词
- **自编码能力**：需要新功能时让 Agent 自己写代码，而不是安装插件
- **树状对话结构**：支持分支，修复 bug 不影响主线
- **高性能**：多线程线程池、读写锁、原子操作
- **生产就绪**：Token 优化、会话持久化、跨平台支持
- **多语言支持**：支持简体中文和英文界面

---

## Features / 核心特性

### Core Tools / 核心工具

| Tool | Description | 描述 |
|------|-------------|------|
| **Read** | Read file contents with pagination support | 读取文件内容，支持分页读取 |
| **Write** | Create new files or overwrite existing files | 创建新文件或覆盖现有文件 |
| **Edit** | Precise string replacement in files | 精确替换文件中的字符串 |
| **Bash** | Execute shell commands with cross-platform support | 执行 shell 命令，跨平台支持 |
| **Serial** | Serial port communication for embedded development | 串口通信工具，用于嵌入式开发 |
| **Browser** | Browser automation (OpenClaw-style visual control) | 浏览器自动化（OpenClaw 风格可视化控制） |
| **Agent** | Discover and manage local AI assistants | Agent 发现和管理 |
| **Hardware** | Motor controllers, sensors, and embedded robotics | **硬件控制、传感器和嵌入式机器人（新增）** |
| **Social** | Connect to Telegram, DingTalk, Feishu for remote control | **社交软件连接（新增）** |

### New in v0.2.0 / v0.2.0 新功能

#### Browser Automation / 浏览器自动化

**OpenClaw-Style Visual Control / OpenClaw 风格可视化控制**

RoboClaw now includes powerful browser automation capabilities similar to OpenClaw:

```
# Open browser and navigate to URL / 打开浏览器并导航
{"action": "open", "browser": "chrome"}
{"action": "navigate", "url": "https://github.com"}

# Take screenshot / 截图
{"action": "screenshot"}

# Click element / 点击元素
{"action": "click", "selector_type": "css", "selector_value": "#submit-button"}

# Type text / 输入文本
{"action": "type", "selector_type": "css", "selector_value": "#search-input", "text": "search query"}

# Execute JavaScript / 执行 JavaScript
{"action": "execute", "script": "document.title='Hello from RoboClaw'"}

# Scroll page / 滚动页面
{"action": "scroll", "x": 0, "y": 500}
```

**Supported Browsers / 支持的浏览器**:
- macOS: Safari, Chrome, Firefox
- Linux: Chrome, Firefox
- Windows: Edge, Chrome, Firefox

#### Agent Discovery & Management / Agent 发现和管理

Automatically discover and manage AI coding assistants installed on your system:

```
# List all installed agents / 列出所有已安装的 Agents
roboclaw agent --list

Output / 输出:
ID: claude_code_vscode
  Name: Claude Code VSCode Extension
  Type: claude_code
  Status: Enabled
  Capabilities: code_completion, chat, code_explanation

ID: cursor_app
  Name: cursor
  Type: cursor
  Status: Enabled
  Capabilities: ide, code_completion, chat, codebase_chat
```

**Supported Agents / 支持的 Agents**:
- Claude Code (VSCode extension)
- Cursor AI IDE
- GitHub Copilot
- OpenAI Codex
- Tabnine
- Blackbox AI
- Sourcegraph Cody
- Replit Ghostwriter
- OpenClaw
- And more...

---

### Embedded Robotics Platform / 嵌入式机器人平台 (v0.3.0 NEW)

**[English]**

RoboClaw now supports embedded robotics development! Transform your Raspberry Pi or Jetson Nano into an intelligent robot controller with natural language commands.

**[中文]**

RoboClaw 现在支持嵌入式机器人开发！将您的 Raspberry Pi 或 Jetson Nano 变成支持自然语言命令的智能机器人控制器。

**Key Features / 核心功能**:

- Hardware Abstraction Layer (HAL) / 硬件抽象层
  - Motor Controllers: RoboClaw, Sabertooth, L298N, PWM drivers / 电机控制器：RoboClaw、Sabertooth、L298N、PWM 驱动
  - Sensors: IMU (MPU6050), LiDAR, Ultrasonic, Encoders / 传感器：IMU (MPU6050)、LiDAR、超声波、编码器
  - Communication: Serial/UART, I2C, SPI / 通信：Serial/UART、I2C、SPI

- Robot Control Skills / 机器人控制技能
  - Motion control: forward, backward, turn, stop / 运动控制：前进、后退、转向、停止
  - Sensor reading: multi-sensor management / 传感器读取：多传感器管理
  - Hardware configuration: JSON-based setup / 硬件配置：基于 JSON 的配置

**Supported Platforms / 支持的平台**:
- Raspberry Pi 4, 3B+ (ARM64/ARM32)
- Jetson Nano, Jetson Orin (ARM64)
- BeagleBone Black (In Development / 开发中)

**Quick Start / 快速开始**:

```bash
# Configure hardware / 配置硬件
cp configs/hardware.json.example ~/.roboclaw/hardware.json
nano ~/.roboclaw/hardware.json

# List hardware / 列出硬件
roboclaw hardware list

# Test connections / 测试连接
roboclaw hardware test

# Interactive robot control / 交互式机器人控制
roboclaw
>>> 前进 50% 速度 2 秒
>>> 左转 90 度
>>> 读取 IMU 数据
>>> 停止
```

**Documentation / 文档**: [Embedded Quick Start Guide](docs/embedded-quickstart.md)

---

### Social Platform Integration / 社交平台集成 (NEW)

**[English]**

RoboClaw now supports connection to social platforms for remote control:

- **Telegram Bot API** - Control RoboClaw via Telegram
- **DingTalk / Feishu** - Enterprise platform integration
- **/link command** - Easy setup wizard for platform connection

**[中文]**

RoboClaw 现在支持连接社交平台进行远程控制：

- **Telegram Bot API** - 通过 Telegram 控制 RoboClaw
- **钉钉 / 飞书** - 企业平台集成
- **/link 命令** - 简单的连接设置向导

**Quick Start / 快速开始**:

```bash
# Link to Telegram / 连接到 Telegram
roboclaw
>>> /link
选择平台: Telegram
输入 Bot Token: <your_bot_token>

# Or use CLI / 或使用命令行
roboclaw social --platform telegram --token <your_bot_token>
```

**Documentation / 文档**: [Social Link Guide](docs/social-link-guide.md)

---

### Agent Collaboration / Agent 协作 (NEW)

**[English]**

RoboClaw can intelligently delegate tasks to specialized agents:

- **Claude Code** - Expert in C++ embedded development
- **Cursor** - General purpose coding assistant
- **OpenClaw** - Visual interaction specialist

Use RoboClaw as your central coordinator, leveraging the strengths of each AI agent.

**[中文]**

RoboClaw 可以智能地将任务委派给专门的 Agents：

- **Claude Code** - C++ 嵌入式开发专家
- **Cursor** - 通用编码助手
- **OpenClaw** - 可视化交互专家

将 RoboClaw 作为您的中央协调器，利用每个 AI Agent 的优势。

---

## Installation / 安装部署

### Quick Install / 快速安装

**Unix (macOS/Linux) / Unix 系统（macOS/Linux）**

```bash
curl -sSL https://raw.githubusercontent.com/yourusername/RoboClaw/main/install | bash
```

Or manually:
```bash
git clone https://github.com/yourusername/RoboClaw.git
cd RoboClaw
./scripts/install
```

**Windows / Windows 系统**

```powershell
# Download and run the installer
irm https://raw.githubusercontent.com/yourusername/RoboClaw/main/install.ps1 | iex
```

Or manually:
```powershell
git clone https://github.com/yourusername/RoboClaw.git
cd RoboClaw
.\scripts\install.ps1
```

The installer will:
- 检测并安装依赖 / Detect and install dependencies
- 自动配置构建系统 / Automatically configure build system
- 编译并安装到 ~/.roboclaw / Compile and install to ~/.roboclaw
- 创建命令行快捷方式 / Create command-line shortcut

After installation, run:
- After install: `roboclaw` or `~/bin/roboclaw`

---

### Prerequisites / 前置要求

| Dependency | Version | macOS | Linux | Windows |
|------------|---------|--------|-------|---------|
| CMake | 3.20+ | Homebrew | Package Manager | Installer |
| C++ Compiler | C++20 | Xcode/Clang | GCC 10+ | MSVC 2019+ |
| Ninja | Latest | Homebrew | Package Manager | Installer |
| nlohmann/json | 3.11+ | Homebrew | Package Manager | vcpkg |

### macOS Installation / macOS 安装

```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install dependencies via Homebrew
brew install cmake ninja nlohmann-json

# Clone repository
git clone https://github.com/yourusername/RoboClaw.git RoboClaw
cd RoboClaw

# Configure with preset
cmake --preset=release

# Build
cmake --build build --config Release

# Run
./build/roboclaw --help
```

### Linux Installation / Linux 安装

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt update
sudo apt install -y cmake ninja-build nlohmann-json3-dev \
    build-essential g++ git

# Clone and build
git clone https://github.com/yourusername/RoboClaw.git RoboClaw
cd RoboClaw
cmake --preset=release
cmake --build build --config Release
./build/roboclaw --help
```

### Windows Installation / Windows 安装

```powershell
# Install vcpkg
git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
.\vcpkg\bootstrap-vcpkg.bat

# Install dependencies
.\vcpkg\vcpkg install nlohmann-json:x64-windows cmake ninja

# Clone and build
git clone https://github.com/yourusername/RoboClaw.git RoboClaw
cd RoboClaw
cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -B build
cmake --build build --config Release
.\build\roboclaw.exe --help
```

---

## Quick Start / 快速开始

### Basic Usage / 基本用法

```bash
# Start interactive mode / 启动交互模式
./build/roboclaw

# Show help / 显示帮助
./build/roboclaw --help

# List discovered agents / 列出发现的 Agents
./build/roboclaw agent --list

# Open browser / 打开浏览器
./build/roboclaw browser --open
```

### Browser Automation Examples / 浏览器自动化示例

```bash
# Navigate to a website / 导航到网站
./build/roboclaw browser --navigate https://github.com

# Take a screenshot / 截图
./build/roboclaw browser --screenshot

# Interactively control browser / 交互式控制浏览器
./build/roboclaw browser
```

### Agent Management Examples / Agent 管理示例

```bash
# List all installed AI agents / 列出所有已安装的 AI agents
./build/roboclaw agent --list

# Show specific agent details / 显示特定 Agent 详情
./build/roboclaw agent --show claude_code_vscode

# Launch an agent / 启动 Agent
./build/roboclaw agent --launch cursor_app
```

---

## Project Structure / 项目结构

```
RoboClaw/
├── CMakeLists.txt              # CMake configuration
├── README.md                   # This file
├── LICENSE                     # MIT License
├── configs/
│   └── hardware.json.example  # Hardware configuration example (NEW)
├── src/
│   ├── main.cpp               # Entry point
│   ├── cli/                   # CLI module
│   │   ├── config_wizard.cpp   # Configuration wizard with language selection
│   │   ├── interactive_mode.cpp # Interactive mode
│   │   └── agent_commands.cpp  # Agent management commands
│   ├── tools/                 # Tools implementation
│   │   ├── tool_base.{h,cpp}   # Base tool class
│   │   ├── read_tool.{h,cpp}   # Read tool
│   │   ├── write_tool.{h,cpp}  # Write tool
│   │   ├── edit_tool.{h,cpp}   # Edit tool
│   │   ├── bash_tool.{h,cpp}   # Bash tool
│   │   ├── serial_tool.{h,cpp}  # Serial port tool
│   │   ├── browser_tool.{h,cpp} # Browser automation
│   │   └── agent_tool.{h,cpp}   # Agent discovery
│   ├── hal/                   # Hardware Abstraction Layer (NEW)
│   │   ├── motor_controller.h  # Motor controller interface
│   │   ├── sensor.h            # Sensor interface
│   │   ├── comm.h              # Communication interface
│   │   ├── hal_exception.h     # Hardware exceptions
│   │   ├── hardware_config.{h,cpp} # Configuration manager
│   │   └── drivers/            # Hardware drivers
│   │       ├── serial_comm.{h,cpp} # Serial communication
│   │       ├── roboclaw_driver.cpp  # RoboClaw driver (TODO)
│   │       └── mpu6050_driver.cpp   # MPU6050 driver (TODO)
│   ├── skills/                # Robot control skills (NEW)
│   │   ├── robot/
│   │   │   ├── motion_skill.{h,cpp}  # Motion control
│   │   │   └── sensor_skill.{h,cpp}  # Sensor reading
│   │   └── embedded/          # Embedded development skills (TODO)
│   ├── agent/
│   │   ├── agent.h/.cpp       # Core Agent class
│   │   ├── tool_executor.h/.cpp # Tool executor (now with 8 tools)
│   │   └── prompt_builder.h/.cpp # Prompt builder
│   ├── llm/                   # LLM provider interface
│   ├── session/                # Session management
│   ├── optimization/           # Token optimization
│   └── utils/                 # Utility classes
├── docs/
│   ├── embedded-quickstart.md # Embedded robotics guide (NEW)
│   └── plans/                 # Design documents
└── tests/
    ├── unit/                  # Unit tests
    ├── integration/           # Integration tests
    └── e2e/                   # End-to-end tests
```

---

## Architecture / 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                     RoboClaw Framework                     │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────┐    ┌───────────┐    ┌────────────────┐   │
│  │   CLI       │    │   Agent   │    │  Browser       │   │
│  │   Module    │◄──►│   Engine   │◄──►│  Automation   │   │
│  └─────────────┘    └─────┬─────┘    └────────────────┘   │
│                          │                                  │
│                   ┌──────▼───────┐                          │
│                   │  Tool        │                          │
│                   │  Executor    │                          │
│                   └──────┬───────┘                          │
│                          │                                  │
│         ┌─────────────┼─────────────┐                       │
│         ▼             ▼             ▼                       │
│   ┌─────────┐ ┌──────────┐ ┌──────────┐                    │
│   │   Read   │ │  Write   │ │   Edit   │                │
│   │   Tool   │ │   Tool   │ │   Tool   │                │
│   └─────────┘ └──────────┘ └──────────┘                    │
│                                                        ┌──────────┐  │
│                                                        │  Bash    │  │
│                                                        │  Tool    │  │
│                                                        └──────────┘  │
│                                                 ┌────────────┐   │
│                                                 │    Serial  │   │
│                                                 │    Tool    │   │
│                                                 └────────────┘   │
│                                                 ┌─────────────┐  │
│                                                 │   Browser   │  │
││                                                 │   Tool     │  │
│                                                 └─────────────┘  │
│                                                 ┌─────────────┐  │
│                                                 │    Agent    │  │
│                                                 │    Tool     │  │
│                                                 └─────────────┘  │
│                                                 ┌─────────────┐  │
│                                                 │  Hardware   │  │
│                                                 │    Tool     │  │ (NEW)
│                                                 └─────┬───────┘  │
│                                                       │         │
│                              ┌────────────────────────┘         │
│                              ▼                                  │
│                   ┌─────────────────────┐                      │
│                   │   HAL & Skills      │                      │
│                   │ ┌─────────────────┐ │                      │
│                   │ │ Motor/Sensor    │ │                      │
│                   │ │ Interfaces      │ │                      │
│                   │ └─────────────────┘ │                      │
│                   │ ┌─────────────────┐ │                      │
│                   │ │ Motion/Sensor   │ │                      │
│                   │ │ Skills          │ │                      │
│                   │ └─────────────────┘ │                      │
│                   └─────────────────────┘                      │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │              Thread Pool (Multithreading)              │  │
│  └──────────────────────────────────────────────────────────┘  │
│                                                                  │
│  ┌─────────────┐  ┌──────────────┐  ┌────────────────┐       │
│  │   Session   │  │ Token        │  │    Agent       │       │
│  │  Manager    │  │  Optimizer   │  │  Discovery     │       │
│  └─────────────┘  └──────────────┘  └────────────────┘       │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## Usage Examples / 使用示例

### Example 1: Browser Automation / 浏览器自动化

```cpp
// Open browser and navigate / 打开浏览器并导航
ToolResult result = toolExecutor->execute("browser", {
    {"action", "open"},
    {"browser", "chrome"}
});

result = toolExecutor->execute("browser", {
    {"action", "navigate"},
    {"url", "https://github.com"}
});

// Take screenshot / 截图
result = toolExecutor->execute("browser", {
    {"action", "screenshot"}
});
```

### Example 2: Agent Discovery / Agent 发现

```cpp
// List all agents / 列出所有 Agents
ToolResult result = toolExecutor->execute("agent", {
    {"action", "list"}
});

// Get agent capabilities / 获取 Agent 能力
result = toolExecutor->execute("agent", {
    {"action", "capabilities"},
    {"agent_id", "claude_code_vscode"}
});
```

### Example 3: Combined Workflow / 组合工作流

```cpp
// 1. Discover agents / 发现 Agents
auto agents = toolExecutor->execute("agent", {{"action", "list"}});

// 2. Open browser / 打开浏览器
toolExecutor->execute("browser", {{"action", "open"}});

// 3. Navigate to repository / 导航到仓库
toolExecutor->execute("browser", {{"action", "navigate"}, {"url", "https://github.com/user/repo"}});

// 4. Read README / 读取 README
toolExecutor->execute("read", {{"file", "README.md"}});
```

### Example 4: Robot Control / 机器人控制（新增）

```cpp
// Hardware control using HAL / 使用 HAL 进行硬件控制
#include "skills/robot/motion_skill.h"
#include "hal/drivers/serial_comm.h"

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
motion.turn(90, 50);      // Turn right 90 degrees / 右转 90 度
motion.stop();            // Emergency stop / 紧急停止
```

---

## Documentation / 文档

- [Embedded Robotics Quick Start](docs/embedded-quickstart.md) - 嵌入式机器人快速入门指南（新增）
- [Social Link Guide](docs/social-link-guide.md) - 社交软件连接指南（新增）
- [Hardware Configuration Guide](configs/hardware.json.example) - 硬件配置示例
- [Design Document](docs/plans/2025-02-20-roboclaw-design.md) - 设计文档
- [Extension Design](docs/plans/2025-02-20-extensions-design.md) - 扩展设计
- [Test Documentation](tests/README.md) - 测试文档

---

## License / 许可证

```
MIT License

Copyright (c) 2025 RoboClaw Contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

---

## Contributing / 贡献指南

Please see [CONTRIBUTING.md](CONTRIBUTING.md) for details.

---

## Acknowledgments / 致谢

**[English]**
- [OpenClaw](https://github.com/OpenClaw) - Original inspiration for visual browser control / 可视化浏览器控制的原始灵感
- [CPR](https://github.com/libcpr/cpr) - HTTP library / HTTP 库
- [nlohmann/json](https://github.com/nlohmann/json) - JSON library / JSON 库
- All contributors / 所有贡献者

**[中文]**
- [OpenClaw](https://github.com/OpenClaw) - 浏览器可视化控制的灵感来源
- [CPR](https://github.com/libcpr/cpr) - HTTP 库
- [nlohmann/json](https://github.com/nlohmann/json) - JSON 库
- 所有开源贡献者

---

<div align="center">

**Made with ❤️ by the RoboClaw Community**

**用 ❤️ 构建 | RoboClaw 社区**

[⭐ Star](https://github.com/yourusername/RoboClaw) &nbsp;&nbsp;
[🍴 Fork](https://github.com/yourusername/RoboClaw/fork) &nbsp;&nbsp;
[📖 Documentation](https://github.com/yourusername/RoboClaw/wiki)

</div>
