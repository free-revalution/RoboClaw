<div align="center">

# RoboPartner

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

RoboPartner is an advanced AI Agent framework written in modern C++ that combines powerful code understanding with browser automation capabilities. Inspired by OpenClaw's visual interaction model and extending it with agent discovery and management, RoboPartner becomes your ultimate AI development companion:

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

RoboPartner 是一个用现代 C++ 编写的高级 AI Agent 框架，结合了强大的代码理解和浏览器自动化能力。灵感来源于 OpenClaw 的可视化交互模型，并扩展了 Agent 发现和管理功能，使其成为您的终极 AI 开发伙伴：

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
| **Browser** | Browser automation (OpenClaw-style visual control) | **浏览器自动化（OpenClaw 风格可视化控制，新增）** |
| **Agent** | Discover and manage local AI assistants | **Agent 发现和管理（新增）** |

### New in v0.2.0 / v0.2.0 新功能

#### Browser Automation / 浏览器自动化

**OpenClaw-Style Visual Control / OpenClaw 风格可视化控制**

RoboPartner now includes powerful browser automation capabilities similar to OpenClaw:

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
{"action": "execute", "script": "document.title='Hello from RoboPartner'"}

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
robopartner agent --list

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
./install
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
.\install.ps1
```

The installer will:
- 检测并安装依赖 / Detect and install dependencies
- 自动配置构建系统 / Automatically configure build system
- 编译并安装到 ~/.robopartner / Compile and install to ~/.robopartner
- 创建命令行快捷方式 / Create command-line shortcut

After installation, run:
- After install: `robopartner` or `~/bin/robopartner`

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
git clone https://github.com/yourusername/RoboClaw.git RoboPartner
cd RoboPartner

# Configure with preset
cmake --preset=release

# Build
cmake --build build --config Release

# Run
./build/robopartner --help
```

### Linux Installation / Linux 安装

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt update
sudo apt install -y cmake ninja-build nlohmann-json3-dev \
    build-essential g++ git

# Clone and build
git clone https://github.com/yourusername/RoboClaw.git RoboPartner
cd RoboPartner
cmake --preset=release
cmake --build build --config Release
./build/robopartner --help
```

### Windows Installation / Windows 安装

```powershell
# Install vcpkg
git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
.\vcpkg\bootstrap-vcpkg.bat

# Install dependencies
.\vcpkg\vcpkg install nlohmann-json:x64-windows cmake ninja

# Clone and build
git clone https://github.com/yourusername/RoboClaw.git RoboPartner
cd RoboPartner
cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -B build
cmake --build build --config Release
.\build\robopartner.exe --help
```

---

## Quick Start / 快速开始

### Basic Usage / 基本用法

```bash
# Start interactive mode / 启动交互模式
./build/robopartner

# Show help / 显示帮助
./build/robopartner --help

# List discovered agents / 列出发现的 Agents
./build/robopartner agent --list

# Open browser / 打开浏览器
./build/robopartner browser --open
```

### Browser Automation Examples / 浏览器自动化示例

```bash
# Navigate to a website / 导航到网站
./build/robopartner browser --navigate https://github.com

# Take a screenshot / 截图
./build/robopartner browser --screenshot

# Interactively control browser / 交互式控制浏览器
./build/robopartner browser
```

### Agent Management Examples / Agent 管理示例

```bash
# List all installed AI agents / 列出所有已安装的 AI agents
./build/robopartner agent --list

# Show specific agent details / 显示特定 Agent 详情
./build/robopartner agent --show claude_code_vscode

# Launch an agent / 启动 Agent
./build/robopartner agent --launch cursor_app
```

---

## Project Structure / 项目结构

```
RoboPartner/
├── CMakeLists.txt              # CMake configuration
├── README.md                   # This file
├── LICENSE                     # MIT License
├── src/
│   ├── main.cpp               # Entry point
│   ├── cli/                   # CLI module
│   │   ├── config_wizard.cpp   # Configuration wizard with language selection
│   │   ├── interactive_mode.cpp # Interactive mode
│   │   └── agent_commands.cpp  # Agent management commands (NEW)
│   ├── tools/                 # Tools implementation
│   │   ├── tool_base.{h,cpp}   # Base tool class
│   │   ├── read_tool.{h,cpp}   # Read tool
│   │   ├── write_tool.{h,cpp}  # Write tool
│   │   ├── edit_tool.{h,cpp}   # Edit tool
│   │   ├── bash_tool.{h,cpp}   # Bash tool
│   │   ├── serial_tool.{h,cpp}  # Serial port tool
│   │   ├── browser_tool.{h,cpp} # Browser automation (NEW)
│   │   └── agent_tool.{h,cpp}   # Agent discovery (NEW)
│   ├── agent/
│   │   ├── agent.h/.cpp       # Core Agent class
│   │   ├── tool_executor.h/.cpp # Tool executor (now with 7 tools)
│   │   └── prompt_builder.h/.cpp # Prompt builder
│   ├── llm/                   # LLM provider interface
│   ├── session/                # Session management
│   ├── optimization/           # Token optimization
│   └── utils/                 # Utility classes
└── tests/
    ├── unit/                  # Unit tests
    ├── integration/           # Integration tests
    └── e2e/                   # End-to-end tests
```

---

## Architecture / 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                     RoboPartner Framework                     │
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

---

## Documentation / 文档

- [Design Document](docs/plans/2025-02-20-roboclaw-design.md) - 设计文档
- [Extension Design](docs/plans/2025-02-20-extensions-design.md) - 扩展设计
- [Test Documentation](tests/README.md) - 测试文档

---

## License / 许可证

```
MIT License

Copyright (c) 2025 RoboPartner Contributors

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

**Made with ❤️ by the RoboPartner Community**

**用 ❤️ 构建 | RoboPartner 社区**

[⭐ Star](https://github.com/yourusername/RoboPartner) &nbsp;&nbsp;
[🍴 Fork](https://github.com/yourusername/RoboPartner/fork) &nbsp;&nbsp;
[📖 Documentation](https://github.com/yourusername/RoboPartner/wiki)

</div>
