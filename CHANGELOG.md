# Changelog / 更新日志

All notable changes to RoboPartner will be documented in this file.
RoboPartner 的所有重要更改都将记录在此文件中。

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).
格式基于 [Keep a Changelog](https://keepachangelog.com/en/1.0.0/)，
本项目遵循 [语义化版本](https://semver.org/spec/v2.0.0.html)。

---

## [0.2.0] - 2025-02-21

### Project Rename / 项目更名
- **Project renamed** from "RoboClaw" to "RoboPartner" / 项目从 "RoboClaw" 更名为 "RoboPartner"
- **Executable renamed** from `roboclaw` to `robopartner` / 可执行文件从 `roboclaw` 更名为 `robopartner`
- **Config directory** changed from `.roboclaw` to `.robopartner` / 配置目录从 `.roboclaw` 改为 `.robopartner`

### Added / 新增
- **Browser Automation Tool**: OpenClaw-style visual browser control / 浏览器自动化工具：OpenClaw 风格可视化浏览器控制
  - Navigate to URLs / 导航到 URL
  - Take screenshots / 截图
  - Click elements by CSS/XPath selectors / 通过 CSS/XPath 选择器点击元素
  - Type text into input fields / 在输入框中输入文本
  - Execute JavaScript / 执行 JavaScript
  - Scroll pages / 滚动页面
  - List and manage tabs / 列出和管理标签页
  - Platform support: macOS (Safari, Chrome, Firefox), Linux (Chrome, Firefox), Windows (Edge, Chrome, Firefox)
  - 平台支持：macOS（Safari、Chrome、Firefox）、Linux（Chrome、Firefox）、Windows（Edge、Chrome、Firefox）
- **Agent Discovery Tool**: Detect and manage local AI coding assistants / Agent 发现工具：检测和管理本地 AI 编程助手
  - Scan for VSCode extensions (Claude Code, Copilot, etc.) / 扫描 VSCode 扩展（Claude Code、Copilot 等）
  - Detect standalone applications (Cursor, Replit, Tabnine) / 检测独立应用（Cursor、Replit、Tabnine）
  - Scan CLI tools (blackbox, tabnine, cody) / 扫描 CLI 工具（blackbox、tabnine、cody）
  - List and show agent capabilities / 列出和显示 Agent 能力
  - Launch detected agents / 启动检测到的 Agents
  - Platform-specific scanning for macOS, Linux, Windows / 针对 macOS、Linux、Windows 的平台特定扫描

### Changed / 更改
- **Tool count increased**: From 5 tools to 7 tools / 工具数量增加：从 5 个工具增加到 7 个工具
- **Core tools now include**: Read, Write, Edit, Bash, Serial, Browser, Agent / 核心工具现在包括：Read、Write、Edit、Bash、Serial、Browser、Agent
- **Session directory**: `.roboclaw/conversations` → `.robopartner/conversations` / 会话目录更改
- **Log file**: `roboclaw.log` → `robopartner.log` / 日志文件更改

### Technical Details / 技术细节
- **Browser implementation**:
  - macOS: AppleScript for browser automation / macOS：使用 AppleScript 进行浏览器自动化
  - Linux: WebDriver (chromedriver, geckodriver) with curl commands / Linux：使用 WebDriver 和 curl 命令
  - Windows: WebDriver (ChromeDriver, MSEdgeDriver) with CreateProcess / Windows：使用 WebDriver 和 CreateProcess
- **Agent detection**:
  - VSCode extensions directory scanning / VSCode 扩展目录扫描
  - Application bundle detection (macOS) / 应用程序包检测（macOS）
  - PATH scanning for CLI tools / CLI 工具的 PATH 扫描
  - Package.json version parsing / Package.json 版本解析

---

## [0.2.1] - 2025-02-22

### Added / 新增
- **One-Command Installation**: Cross-platform automated installation / 一键安装：跨平台自动化安装
  - Unix (macOS/Linux) installer script with dependency detection / Unix 安装脚本，支持依赖检测
  - Windows PowerShell installer with Visual Studio detection / Windows PowerShell 安装脚本，支持 VS 检测
  - Automatic build and installation to ~/.robopartner / 自动编译并安装到 ~/.robopartner
  - PATH setup with ~/bin symlink / PATH 配置，通过 ~/bin 符号链接
- **Terminal UI Module**: Professional terminal interface utilities / 终端 UI 模块：专业终端界面工具
  - Terminal class: width/height detection, screen clearing, cursor control / Terminal 类：宽高检测、清屏、光标控制
  - UI class: draw boxes, separators, tables, progress bars, spinners / UI 类：绘制边框、分隔线、表格、进度条、加载动画
  - ANSI color support with tech blue branding / ANSI 颜色支持，科技蓝主题
  - Cross-platform: Windows (Console API), Unix (ioctl) / 跨平台：Windows 控制台 API、Unix ioctl
  - Multiple box styles: single, double, rounded, ASCII, bold / 多种边框样式
- **Claude Code-Style Welcome Screen**: ASCII art logo with colored borders / Claude Code 风格欢迎界面：ASCII 艺术字 Logo 彩色边框
  - "RoboPartner v0.2.0" ASCII art logo / "RoboPartner v0.2.0" ASCII 艺术 Logo
  - Current model and provider display / 当前模型和提供商显示
  - Usage tips section / 使用提示部分
  - Adaptive width based on terminal size / 根据终端大小自适应宽度
- **Slash Command System**: In-chat command interface / 斜杠命令系统：聊天内命令界面
  - `/help` - Show available commands / 显示可用命令
  - `/config` - Edit configuration file / 编辑配置文件
  - `/clear` - Clear conversation and redraw welcome screen / 清空对话并重绘欢迎界面
  - `/agent` - Manage AI Agents (list, show, launch) / 管理 AI Agents
  - `/browser` - Browser automation commands / 浏览器自动化命令
  - `Ctrl+D` - Exit the application / 退出应用
- **Colored Output**: ANSI color-coded messages / 彩色输出：ANSI 颜色编码消息
  - Errors in RED / 错误信息红色
  - User prompts in CYAN / 用户提示青色
  - Tool calls in YELLOW / 工具调用黄色
  - Success messages in GREEN / 成功消息绿色
  - System messages in GRAY / 系统消息灰色

### Changed / 更改
- **Welcome screen redesign**: From simple text banner to professional UI / 欢迎界面重新设计：从简单文本横幅改为专业 UI
- **Prompt style**: From "你:" to modern ">>>" prompt / 提示符样式：从 "你:" 改为现代 ">>>" 提示符
- **Help display**: From text list to formatted box / 帮助显示：从文本列表改为格式化边框
- **Command routing**: Dedicated `handleSlashCommand()` for slash commands / 命令路由：专用 `handleSlashCommand()` 处理斜杠命令

### Technical / 技术细节
- **New files**:
  - `install` - Unix installation script (117 lines) / Unix 安装脚本
  - `install.ps1` - Windows PowerShell script (400+ lines) / Windows PowerShell 脚本
  - `src/utils/terminal.h` - Terminal utility header (270+ lines) / 终端工具头文件
  - `src/utils/terminal.cpp` - Terminal utility implementation (400+ lines) / 终端工具实现
- **Security improvements**:
  - URL validation in install.ps1 / PowerShell 安装脚本 URL 验证
  - Path traversal protection / 路径遍历保护
  - Commit hash verification option / 提交哈希验证选项
  - WhatIf support for safe preview / WhatIf 支持安全预览

---

## [0.3.0] - 2025-02-22

### Added - 嵌入式机器人平台 / Embedded Robotics Platform

#### Hardware Abstraction Layer (HAL) / 硬件抽象层
- **IMotorController interface** / 电机控制器接口：支持 RoboClaw、Sabertooth、L298N、PWM drivers
- **ISensor interface** / 传感器接口：支持 IMU (MPU6050)、LiDAR (RPLIDAR)、超声波 (HC-SR04)、编码器
- **IComm interface** / 通信接口：支持 Serial、I2C、SPI、CAN bus
- **SerialComm driver** / 串口驱动：跨平台串口通信实现（Linux/macOS termios、Windows Win32 API）
- **Hardware exception hierarchy** / 硬件异常层次：HardwareException、CommException、SensorException、MotorException

#### Robot Control Skills / 机器人控制技能
- **MotionSkill** / 运动控制技能：差速驱动机器人运动控制
  - `forward(speed, duration)` - 前进 / Move forward
  - `backward(speed, duration)` - 后退 / Move backward
  - `turn(angle, speed)` - 转向 / Turn by angle
  - `stop()` - 紧急停止 / Emergency stop
- **SensorSkill** / 传感器技能：多传感器管理和读取
  - `registerSensor(name, sensor)` - 注册传感器 / Register sensor
  - `readSensor(name)` - 读取指定传感器 / Read specific sensor
  - `readAll()` - 读取所有传感器 / Read all sensors
  - `isAvailable(name)` - 检查传感器可用性 / Check sensor availability

#### Hardware Configuration / 硬件配置
- **HardwareConfig manager** / 硬件配置管理器：从 JSON 文件加载硬件配置
- **Configuration format** / 配置格式：JSON 格式硬件配置文件
- **Example configuration** / 示例配置：`configs/hardware.json.example`

#### CLI Enhancements / CLI 增强
- `robopartner hardware list` - 列出所有已配置硬件 / List all configured hardware
- `robopartner hardware test` - 测试硬件连接 / Test hardware connections

#### Documentation / 文档
- **Embedded quickstart guide** / 嵌入式快速入门指南：`docs/embedded-quickstart.md`
- **Hardware configuration example** / 硬件配置示例：`configs/hardware.json.example`
- **Updated README** / 更新的 README：添加嵌入式平台概述

### Technical Improvements / 技术改进
- **Cross-platform serial communication** / 跨平台串口通信：使用原生 API 实现，无第三方依赖
- **Hardware abstraction design** / 硬件抽象设计：三层架构（Core、HAL、Application）
- **Exception handling** / 异常处理：专门的硬件异常类型
- **Unit test coverage** / 单元测试覆盖：所有 HAL 接口和技能都有测试

### Dependencies / 依赖变更
- **No new external dependencies** / 无新增外部依赖
- Uses existing `json/json.h` for configuration / 使用现有的 json/json.h 进行配置

### Architecture / 架构
```
Application Layer    → Robot Control Skills, Embedded Dev Skills, ROS Bridge
Hardware Abstraction → Motor Controllers, Sensors, Communication, Config
Core Layer           → AI Engine, Task Parser, Code Generator, Session
```

---

## [0.4.0] - 2025-02-22

### 新增 - 社交软件连接 / Social Platform Connection

#### 社交平台集成 / Social Platform Integration
- **/link 命令** - 连接 Telegram、钉钉、飞书 / Connect Telegram, DingTalk, Feishu
- **双向消息通信** / Bidirectional message communication
- **远程控制终端功能** / Remote terminal control functionality

#### Telegram 适配器 / Telegram Adapter
- **Bot API 集成** / Bot API integration
- **Long polling 消息接收** / Long polling message reception
- **文件和消息发送** / File and message sending

#### 任务协调器 / Task Coordinator
- **智能任务分析** / Intelligent task analysis
- **Agent 能力矩阵** / Agent capability matrix
- **自动 Agent 选择** / Automatic agent selection

#### Agent Bridge 系统 / Agent Bridge System
- **Claude Code Bridge** / Claude Code Bridge
- **统一 Agent 通信接口** / Unified agent communication interface
- **任务委托和结果接收** / Task delegation and result reception

### 文档 / Documentation
- **社交软件连接指南** / Social platform connection guide
- **Agent 协作架构文档** / Agent collaboration architecture documentation

---

## [Unreleased]

### Added / 新增
- **Language selection**: Choose between Simplified Chinese and English during setup / 语言选择：配置时选择简体中文或英文
- **Serial port tool**: Cross-platform serial communication for embedded development / 串口工具：跨平台串口通信，支持嵌入式开发
  - List available serial ports / 列出可用串口
  - Open, configure, read, write, and close serial ports / 打开、配置、读取、写入和关闭串口
  - Support for Linux (ttyUSB*, ttyACM*), macOS (cu.usbserial*), Windows (COM*) / 支持 Linux、macOS、Windows
- Thread pool implementation for concurrent task execution / 并发任务执行的线程池实现
- Token optimization with LRU cache / 带 LRU 缓存的 Token 优化
- Skill system with parser, registry, and executor / 带解析器、注册表和执行器的技能系统
- Session management with tree-structured conversations / 树状对话结构的会话管理
- Multi-platform support (macOS, Linux, Windows) / 多平台支持
- VSCode integration with CMake presets / 带 CMake 预设的 VSCode 集成

### Changed / 更改
- Refactored tool result structure (error → error_message) / 重构工具结果结构
- Updated to C++20 standard / 更新到 C++20 标准
- Improved thread safety with read-write locks / 用读写锁改进线程安全

### Fixed / 修复
- Fixed compilation errors across all modules / 修复所有模块的编译错误
- Fixed header include paths / 修复头文件包含路径
- Fixed CMake configuration for Homebrew dependencies / 修复 Homebrew 依赖的 CMake 配置

---

## [0.1.0] - 2025-02-21

### Added / 新增
- Initial project structure / 初始项目结构
- CMake build configuration / CMake 构建配置
- Four core tools: Read, Write, Edit, Bash / 四个核心工具
- LLM provider interface (Anthropic, OpenAI) / LLM 提供商接口
- Agent engine with conversation history / 带对话历史的 Agent 引擎
- Interactive CLI mode / 交互式 CLI 模式
- Configuration wizard / 配置向导
- Token estimation and optimization / Token 估算和优化

### Features / 特性
- Cross-platform tool execution / 跨平台工具执行
- Stream processing support / 流式处理支持
- Session persistence / 会话持久化
- Modular architecture / 模块化架构
