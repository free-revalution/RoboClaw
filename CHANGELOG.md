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
