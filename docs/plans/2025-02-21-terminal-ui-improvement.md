# Terminal UI & Installation Experience Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** 创建跨平台安装脚本和 Claude Code 风格的终端欢迎界面，提升用户体验

**Architecture:**
- 安装脚本：Shell (Unix) + PowerShell (Windows)，自动检测平台和依赖
- 终端 UI：独立模块负责颜色、边框、自适应宽度
- 交互模式：重构为滚动式布局，支持聊天内斜杠命令

**Tech Stack:** Bash, PowerShell, C++20, ANSI 颜色码, ioctl (终端检测)

---

## Task 1: 创建 Unix 安装脚本 (install.sh)

**Files:**
- Create: `install`

**Step 1: Write the install script header**

```bash
#!/usr/bin/env bash
# RoboClaw Installation Script
# Supports: macOS, Linux

set -e

INSTALL_DIR="${INSTALL_DIR:-$HOME/.roboclaw}"
BIN_DIR="$HOME/bin"
REPO_URL="https://github.com/xxx/RoboClaw.git"
```

**Step 2: Add platform detection**

```bash
detect_platform() {
    case "$(uname -s)" in
        Darwin) echo "macOS"; return 0 ;;
        Linux)  echo "Linux"; return 0 ;;
        *)       echo "Unsupported"; return 1 ;;
    esac
}

PLATFORM=$(detect_platform)
echo ">>> 检测到系统: $PLATFORM"
```

**Step 3: Add dependency checking**

```bash
check_dependencies() {
    local missing=()

    command -v cmake >/dev/null 2>&1 || missing+=("cmake")
    command -v g++ >/dev/null 2>&1 || missing+=("g++")

    if [ ${#missing[@]} -gt 0 ]; then
        echo ">>> 缺少依赖: ${missing[*]}"
        if [ "$PLATFORM" = "macOS" ]; then
            echo "安装命令: brew install ${missing[*]}"
        else
            echo "安装命令: sudo apt-get install ${missing[*]}"
        fi
        return 1
    fi
}
```

**Step 4: Add build and install functions**

```bash
build_robopartner() {
    TEMP_DIR=$(mktemp -d)
    cd "$TEMP_DIR"

    echo ">>> 正在下载源码..."
    git clone "$REPO_URL" .

    echo ">>> 正在编译..."
    mkdir build && cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make -j$(sysctl -n hw.ncpu 2>/dev/null || echo 2)

    echo ">>> 正在安装..."
    mkdir -p "$INSTALL_DIR"
    cp robopartner "$INSTALL_DIR/"
    chmod +x "$INSTALL_DIR/robopartner"

    # Create symlink
    mkdir -p "$BIN_DIR"
    ln -sf "$INSTALL_DIR/robopartner" "$BIN_DIR/robopartner"

    cd -
    rm -rf "$TEMP_DIR"
}
```

**Step 5: Add main execution flow**

```bash
main() {
    echo "=========================================="
    echo "   RoboPartner 安装程序"
    echo "=========================================="
    echo ""

    check_dependencies
    build_robopartner

    echo ""
    echo ">>> 安装完成！"
    echo ""
    echo "运行: robopartner"
    echo ""

    # Setup PATH
    if ! echo "$PATH" | grep -q "$BIN_DIR"; then
        echo "添加到 ~/.zshrc 或 ~/.bashrc:"
        echo "  export PATH=\"$BIN_DIR:\$PATH\""
    fi
}

main "$@"
```

**Step 6: Make executable and test**

Run:
```bash
chmod +x install
# Review the script
cat install
```

**Step 7: Commit**

```bash
git add install
git commit -m "feat: add Unix installation script

- Cross-platform detection (macOS/Linux)
- Dependency checking with helpful install commands
- Builds from source and installs to ~/.robopartner
- Creates symlink in ~/bin"
```

---

## Task 2: 创建 Windows 安装脚本 (install.ps1)

**Files:**
- Create: `install.ps1`

**Step 1: Write the PowerShell script header**

```powershell
# RoboPartner Installation Script for Windows
# Requires: PowerShell 5.1+

param(
    [string]$InstallDir = "$env:USERPROFILE\.robopartner",
    [string]$RepoUrl = "https://github.com/xxx/RoboClaw/archive/main.zip"
)

$ErrorActionPreference = "Stop"
```

**Step 2: Add dependency checking**

```powershell
function Test-Dependencies {
    $missing = @()

    if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
        $missing += "cmake"
    }
    if (-not (Get-Command cl -ErrorAction SilentlyContinue)) {
        $missing += "Visual Studio Build Tools"
    }

    if ($missing.Count -gt 0) {
        Write-Host ">>> 缺少依赖: $($missing -join ', ')" -ForegroundColor Red
        Write-Host ""
        Write-Host "安装方式:" -ForegroundColor Yellow
        Write-Host "  cmake: 下载安装或使用 choco install cmake"
        Write-Host "  VS Build Tools: https://visualstudio.microsoft.com/downloads/"
        return $false
    }
    return $true
}
```

**Step 3: Add build function**

```powershell
function Build-RoboPartner {
    $tempDir = Join-Path $env:TEMP "robopartner-build"

    Write-Host ">>> 正在下载源码..." -ForegroundColor Cyan
    Invoke-WebRequest -Uri $RepoUrl -OutFile "$tempDir.zip"
    Expand-Archive "$tempDir.zip" -DestinationPath $tempDir -Force

    Write-Host ">>> 正在编译..." -ForegroundColor Cyan
    $buildDir = Join-Path $tempDir "RoboClaw-main\build"
    New-Item -ItemType Directory -Path $buildDir -Force | Out-Null
    Push-Location $buildDir

    cmake .. -DCMAKE_BUILD_TYPE=Release
    cmake --build . --config Release

    Write-Host ">>> 正在安装..." -ForegroundColor Cyan
    New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null
    Copy-Item "Release\robopartner.exe" -Destination "$InstallDir\robopartner.exe"

    Pop-Location
    Remove-Item -Recurse -Force $tempDir, "$tempDir.zip"
}
```

**Step 4: Add main execution**

```powershell
function Main {
    Write-Host "==========================================" -ForegroundColor Cyan
    Write-Host "   RoboPartner 安装程序" -ForegroundColor Cyan
    Write-Host "==========================================" -ForegroundColor Cyan
    Write-Host ""

    if (-not (Test-Dependencies)) {
        exit 1
    }

    Build-RoboPartner

    Write-Host ""
    Write-Host ">>> 安装完成!" -ForegroundColor Green
    Write-Host ""
    Write-Host "运行: robopartner" -ForegroundColor Cyan
}

Main
```

**Step 5: Test script syntax**

Run: `pwsh -NoProfile install.ps1 -WhatIf`

**Step 6: Commit**

```bash
git add install.ps1
git commit -m "feat: add Windows installation script (PowerShell)

- Checks for cmake and Visual Studio Build Tools
- Downloads, compiles, and installs to ~/.robopartner
- Provides helpful installation instructions for missing dependencies"
```

---

## Task 3: 创建终端工具模块 (terminal.h/cpp)

**Files:**
- Create: `src/utils/terminal.h`
- Create: `src/utils/terminal.cpp`

**Step 1: Write terminal.h header**

```cpp
// src/utils/terminal.h
#ifndef ROBOCLAW_UTILS_TERMINAL_H
#define ROBOCLAW_UTILS_TERMINAL_H

#include <string>
#include <optional>

namespace roboclaw {

// ANSI 颜色码
namespace Color {
    constexpr const char* RESET     = "\033[0m";
    constexpr const char* RED       = "\033[91m";
    constexpr const char* GREEN     = "\033[92m";
    constexpr const char* YELLOW    = "\033[93m";
    constexpr const char* BLUE      = "\033[94m";
    constexpr const char* MAGENTA   = "\033[95m";
    constexpr const char* CYAN      = "\033[96m";
    constexpr const char* WHITE     = "\033[97m";
    constexpr const char* GRAY      = "\033[90m";
    constexpr const char* BG_GRAY   = "\033[100m";
}

// 终端工具类
class Terminal {
public:
    // 获取终端宽度
    static int getWidth();

    // 获取终端高度
    static int getHeight();

    // 清屏
    static void clear();

    // 移动光标到行首
    static void cursorToStart();

    // 隐藏光标
    static void hideCursor();

    // 显示光标
    static void showCursor();

    // 生成边框线
    static std::string horizontalLine(char ch, int width);

    // 生成居中文本
    static std::string center(const std::string& text, int width);

    // 检测颜色支持
    static bool supportsColor();

    // 检测 Unicode 支持
    static bool supportsUnicode();
};

// UI 绘图工具
class UI {
public:
    // 绘制带边框的文本框
    static void drawBox(const std::string& content,
                       const std::string& title = "",
                       const std::string& color = Color::BLUE);

    // 绘制分隔线
    static void drawSeparator(const std::string& style = "single");

    // 绘制欢迎 Logo
    static void drawLogo();

    // 绘制模型信息
    static void drawModelInfo(const std::string& model,
                             const std::string& provider);

    // 绘制用法提示
    static void drawUsageTips();
};

} // namespace roboclaw

#endif // ROBOCLAW_UTILS_TERMINAL_H
```

**Step 2: Write terminal.cpp implementation**

```cpp
// src/utils/terminal.cpp
#include "terminal.h"
#include "../storage/config_manager.h"
#include <iostream>
#include <sstream>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <io.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#endif

namespace roboclaw {

int Terminal::getWidth() {
#ifdef PLATFORM_WINDOWS
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    return csbi.srWindow.Right - csbi.srWindow.Left + 1;
#else
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_col;
#endif
}

int Terminal::getHeight() {
#ifdef PLATFORM_WINDOWS
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    return csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
#else
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_row;
#endif
}

void Terminal::clear() {
#ifdef PLATFORM_WINDOWS
    system("cls");
#else
    system("clear");
#endif
}

void Terminal::cursorToStart() {
    std::cout << "\r" << std::flush;
}

void Terminal::hideCursor() {
    std::cout << "\033[?25l" << std::flush;
}

void Terminal::showCursor() {
    std::cout << "\033[?25h" << std::flush;
}

std::string Terminal::horizontalLine(char ch, int width) {
    return std::string(width, ch);
}

std::string Terminal::center(const std::string& text, int width) {
    int padding = width - text.length();
    if (padding <= 0) return text;
    int leftPad = padding / 2;
    int rightPad = padding - leftPad;
    return std::string(leftPad, ' ') + text + std::string(rightPad, ' ');
}

bool Terminal::supportsColor() {
#ifdef PLATFORM_WINDOWS
    // Windows 10+ 支持 ANSI 颜色
    return true;
#else
    // 检查 TERM 环境变量
    const char* term = std::getenv("TERM");
    if (term && std::string(term) != "dumb") {
        return true;
    }
    return false;
#endif
}

bool Terminal::supportsUnicode() {
#ifdef PLATFORM_WINDOWS
    // Windows 10+ 支持 Unicode
    return true;
#else
    // Unix 系统通常支持
    return true;
#endif
}

void UI::drawBox(const std::string& content,
                 const std::string& title,
                 const std::string& color) {
    int width = Terminal::getWidth();
    if (width < 40) width = 40;
    if (width > 100) width = 100;

    std::string border(width, '═');
    std::cout << color << "╔" << border << "╗" << Color::RESET << "\n";

    if (!title.empty()) {
        std::string titleLine = "║ " + title + std::string(width - title.length() - 2, ' ') + " ║";
        std::cout << color << titleLine << Color::RESET << "\n";
        std::cout << color << "╠" << border << "╣" << Color::RESET << "\n";
    }

    std::cout << color << "║" << Color::RESET << " "
              << Terminal::center(content, width - 2)
              << color << " ║" << Color::RESET << "\n";
    std::cout << color << "╚" << border << "╝" << Color::RESET << "\n";
}

void UI::drawSeparator(const std::string& style) {
    int width = Terminal::getWidth();
    if (style == "double") {
        std::cout << Color::GRAY << std::string(width, '=') << Color::RESET << "\n";
    } else {
        std::cout << Color::GRAY << std::string(width, '-') << Color::RESET << "\n";
    }
}

void UI::drawLogo() {
    std::cout << Color::CYAN << R"(
   ███╗   ██╗███████╗██╗  ██╗██╗   ██╗███████╗    ██████╗ ███████╗
   ████╗  ██║██╔════╝██║ ██╔╝██║   ██║██╔════╝    ██╔══██╗██╔════╝
   ██╔██╗ ██║█████╗  █████╔╝ ██║   ██║███████╗    ██████╔╝█████╗
   ██║╚██╗██║██╔══╝  ██╔═██╗ ██║   ██║╚════██║    ██╔══██╗██╔══╝
   ██║ ╚████║███████╗██║  ██╗╚██████╔╝███████║    ██║  ██║███████╗
   ╚═╝  ╚═══╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝    ╚═╝  ╚═╝╚══════╝
)" << Color::RESET;

    std::cout << Color::BLUE << "                          v0.2.0" << Color::RESET << "\n\n";
}

void UI::drawModelInfo(const std::string& model, const std::string& provider) {
    std::cout << Color::GRAY << "   ═════════════════════════════════════════════════════════════════   " << Color::RESET << "\n";
    std::cout << "   C++ AI Agent Framework with Browser Automation" << "\n";
    std::cout << "   AI Agent 框架与浏览器自动化" << "\n";
    std::cout << Color::GRAY << "   ═════════════════════════════════════════════════════════════════   " << Color::RESET << "\n\n";

    std::cout << "   [当前模型] " << Color::YELLOW << model << Color::RESET << "\n";
    std::cout << "   [提供商]   " << Color::CYAN << provider << Color::RESET << "\n\n";
}

void UI::drawUsageTips() {
    std::cout << Color::GRAY << "════════════════════════════════════════════════════════════════════" << Color::RESET << "\n";
    std::cout << "  用法提示\n";
    std::cout << Color::GRAY << "════════════════════════════════════════════════════════════════════" << Color::RESET << "\n\n";

    std::cout << "  直接输入消息开始对话\n";
    std::cout << "  " << Color::GREEN << "/help" << Color::RESET << "      查看所有命令\n";
    std::cout << "  " << Color::GREEN << "/config" << Color::RESET << "    修改配置\n";
    std::cout << "  " << Color::GREEN << "/clear" << Color::RESET << "     清空对话\n";
    std::cout << "  " << Color::GREEN << "/agent" << Color::RESET << "     管理 AI Agents\n";
    std::cout << "  " << Color::GREEN << "/browser" << Color::RESET << "   浏览器自动化\n";
    std::cout << "  " << Color::RED << "Ctrl+D" << Color::RESET << "     退出程序\n\n";

    std::cout << Color::GRAY << "════════════════════════════════════════════════════════════════════" << Color::RESET << "\n\n";
}

} // namespace roboclaw
```

**Step 3: Update CMakeLists.txt to add terminal.cpp**

Add to SOURCES list:
```cmake
# 工具类
src/utils/logger.cpp
src/utils/thread_pool.cpp
src/utils/terminal.cpp
```

**Step 4: Test compile**

Run: `ninja -C build`

**Step 5: Commit**

```bash
git add src/utils/terminal.h src/utils/terminal.cpp CMakeLists.txt
git commit -m "feat: add terminal utility module

- Terminal class: get width/height, clear screen, cursor control
- UI class: draw box, separators, logo, model info
- Cross-platform: Windows (Console API), Unix (ioctl)
- ANSI color support detection"
```

---

## Task 4: 更新 interactive_mode.h 新接口

**Files:**
- Modify: `src/cli/interactive_mode.h`

**Step 1: Add new methods to InteractiveMode class**

```cpp
// 在 InteractiveMode 类中添加:

// 显示欢迎界面
void showWelcome();

// 显示输入提示
void showPrompt();

// 处理斜杠命令
bool handleSlashCommand(const std::string& command);

// 命令处理函数
bool cmdHelp();
bool cmdConfig();
bool cmdClear();
bool cmdAgent(const std::string& args);
bool cmdBrowser(const std::string& args);
```

**Step 2: Commit**

```bash
git add src/cli/interactive_mode.h
git commit -m "refactor: add new UI method declarations to InteractiveMode

- showWelcome(): display Claude Code-style welcome screen
- showPrompt(): display user input prompt with colors
- handleSlashCommand(): route /help, /config, /clear, etc.
- Individual command handlers: cmdHelp(), cmdConfig(), etc."
```

---

## Task 5: 重写 interactive_mode.cpp 实现新界面

**Files:**
- Modify: `src/cli/interactive_mode.cpp`

**Step 1: Update includes**

```cpp
#include "interactive_mode.h"
#include "../utils/logger.h"
#include "../utils/terminal.h"  // 新增
#include "../storage/config_manager.h"
#include <iostream>
#include <sstream>
#include <algorithm>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif
```

**Step 2: Rewrite showWelcome()**

```cpp
void InteractiveMode::showWelcome() {
    using namespace Color;

    // 清屏
    Terminal::clear();

    int width = Terminal::getWidth();
    if (width < 60) width = 60;

    // 顶部边框
    std::cout << CYAN << "╔" << std::string(width - 2, '═') << "╗" << RESET << "\n";
    std::cout << CYAN << "║" << RESET << std::string(width - 2, ' ') << CYAN << "║" << RESET << "\n";

    // Logo
    UI::drawLogo();

    // 模型信息
    const auto& config = config_manager_.getConfig();
    std::string modelName = config.default_config.model;
    std::string providerName = ConfigManager::providerToString(config.default_config.provider);
    UI::drawModelInfo(modelName, providerName);

    // 底部边框
    std::cout << CYAN << "║" << RESET << std::string(width - 2, ' ') << CYAN << "║" << RESET << "\n";
    std::cout << CYAN << "╚" << std::string(width - 2, '═') << "╝" << RESET << "\n\n";

    // 用法提示
    UI::drawUsageTips();
}
```

**Step 3: Rewrite showPrompt()**

```cpp
void InteractiveMode::showPrompt() {
    std::cout << Color::GRAY << "────────────────────────────────────────────────────────────────────" << Color::RESET << "\n";
    std::cout << "  " << Color::GREEN << "you" << Color::RESET << " > " << std::flush;
}
```

**Step 4: Add handleSlashCommand()**

```cpp
bool InteractiveMode::handleSlashCommand(const std::string& command) {
    std::istringstream iss(command);
    std::string cmd;
    iss >> cmd;

    if (cmd == "/help") return cmdHelp();
    if (cmd == "/config") return cmdConfig();
    if (cmd == "/clear") return cmdClear();

    if (cmd == "/agent") {
        std::string args;
        std::getline(iss, args);
        return cmdAgent(args);
    }

    if (cmd == "/browser") {
        std::string args;
        std::getline(iss, args);
        return cmdBrowser(args);
    }

    std::cout << Color::RED << "未知命令: " << cmd << Color::RESET << "\n";
    std::cout << "输入 /help 查看可用命令\n";
    return true;  // 继续运行
}
```

**Step 5: Implement command handlers**

```cpp
bool InteractiveMode::cmdHelp() {
    std::cout << Color::CYAN << "\n可用命令:\n" << Color::RESET;
    std::cout << "  /help      显示此帮助\n";
    std::cout << "  /config    打开配置编辑\n";
    std::cout << "  /clear     清空当前对话\n";
    std::cout << "  /agent     管理 AI Agents\n";
    std::cout << "  /browser   浏览器自动化\n";
    std::cout << "  Ctrl+D     退出程序\n\n";
    return true;
}

bool InteractiveMode::cmdConfig() {
    std::string configPath = ConfigManager::getConfigPath();
    std::cout << "\n配置文件: " << configPath << "\n";
    std::cout << "正在打开编辑器...\n";

#ifdef PLATFORM_MACOS
    system(("open \"" + configPath + "\"").c_str());
#elif defined(PLATFORM_LINUX)
    system(("xdg-open \"" + configPath + "\"").c_str());
#elif defined(PLATFORM_WINDOWS)
    system(("start \"\" \"" + configPath + "\"").c_str());
#endif

    return true;
}

bool InteractiveMode::cmdClear() {
    // 清除当前会话
    auto session = session_manager_->getCurrentSession();
    if (session) {
        session->clear();
        std::cout << Color::GREEN << "对话已清空\n" << Color::RESET;
    }
    return true;
}

bool InteractiveMode::cmdAgent(const std::string& args) {
    std::istringstream iss(args);
    std::string action;
    iss >> action;

    if (action.empty() || action == "list") {
        // 列出 agents
        json params = {{"action", "list"}};
        auto result = agent_->getToolExecutor()->executeTool("agent", params);
        std::cout << result.content << "\n";
    } else if (action == "show") {
        std::string agentId;
        iss >> agentId;
        json params = {{"action", "show"}, {"agent_id", agentId}};
        auto result = agent_->getToolExecutor()->executeTool("agent", params);
        std::cout << result.content << "\n";
    }

    return true;
}

bool InteractiveMode::cmdBrowser(const std::string& args) {
    std::istringstream iss(args);
    std::string action;
    iss >> action;

    if (action.empty() || action == "open") {
        json params = {{"action", "open"}};
        auto result = agent_->getToolExecutor()->executeTool("browser", params);
        std::cout << result.content << "\n";
    } else if (action == "navigate") {
        std::string url;
        iss >> url;
        json params = {{"action", "navigate"}, {"url", url}};
        auto result = agent_->getToolExecutor()->executeTool("browser", params);
        std::cout << result.content << "\n";
    }

    return true;
}
```

**Step 6: Update run() to use slash commands**

```cpp
void InteractiveMode::run() {
    showWelcome();

    // 设置会话目录
    session_manager_->setSessionsDir(".robopartner/conversations");

    auto session = session_manager_->getOrCreateLatestSession();
    session_manager_->setCurrentSession(session);

    while (!should_exit_) {
        showPrompt();
        std::string input = readInput();

        if (input.empty()) continue;

        // 处理斜杠命令
        if (input[0] == '/') {
            handleSlashCommand(input);
            continue;
        }

        // 处理普通消息
        processMessage(input);
        saveCurrentSession();
    }
}
```

**Step 7: Update processMessage() display**

```cpp
void InteractiveMode::displayResponse(const AgentResponse& response) {
    if (!response.success) {
        std::cout << "\n" << Color::RED << "错误: " << response.error << Color::RESET << "\n\n";
        return;
    }

    std::cout << "\n" << Color::BLUE << "Assistant >" << Color::RESET << "\n\n";

    // 显示内容
    if (!response.content.empty()) {
        std::cout << response.content << "\n\n";
    }

    // 显示工具调用
    if (!response.tool_calls.empty()) {
        std::cout << Color::GRAY << "  [工具调用]" << Color::RESET << "\n";
        for (const auto& call : response.tool_calls) {
            std::cout << Color::YELLOW << "    " << call.name << Color::RESET;
            std::cout << " → " << call.arguments.dump().substr(0, 50) << "...\n";
        }
        std::cout << "\n";
    }
}
```

**Step 8: Test compile**

Run: `ninja -C build`

**Step 9: Commit**

```bash
git add src/cli/interactive_mode.cpp
git commit -m "feat: implement new terminal UI with Claude Code-style welcome

- Adaptive width borders using Terminal class
- ASCII art logo with color scheme
- Model info display with provider details
- Usage tips section
- Slash command system (/help, /config, /clear, /agent, /browser)
- Colored user/AI message display
- Clean separator lines between messages"
```

---

## Task 6: 更新 CMakeLists.txt 添加新文件

**Files:**
- Modify: `CMakeLists.txt`

**Step 1: Add terminal.cpp to SOURCES**

```cmake
# 工具类
src/utils/logger.cpp
src/utils/thread_pool.cpp
src/utils/terminal.cpp
```

**Step 2: Test build**

Run: `ninja -C build`

**Step 3: Commit**

```bash
git add CMakeLists.txt
git commit -m "build: add terminal.cpp to build sources"
```

---

## Task 7: 测试完整功能

**Files:**
- Test: Manual testing

**Step 1: Test welcome screen**

Run: `./build/robopartner`

Expected output:
- Colored ASCII art logo
- Model info display
- Usage tips
- Clean borders

**Step 2: Test slash commands**

Input: `/help`
Expected: Command list

Input: `/agent list`
Expected: List of installed agents

Input: `/clear`
Expected: "对话已清空"

**Step 3: Test conversation**

Input: `你好`
Expected: Colored AI response

**Step 4: Test exit**

Input: `Ctrl+D`
Expected: Clean exit

---

## Task 8: 更新 README.md 文档

**Files:**
- Modify: `README.md`

**Step 1: Add installation section**

```markdown
## 安装 / Installation

### macOS / Linux

```bash
curl -fsSL https://raw.githubusercontent.com/xxx/RoboClaw/main/install | sh
```

### Windows (PowerShell)

```powershell
irm https://raw.githubusercontent.com/xxx/RoboClaw/main/install.ps1 | iex
```

### 从源码编译

```bash
git clone https://github.com/xxx/RoboClaw.git
cd RoboClaw
mkdir build && cd build
cmake ..
make -j
sudo make install
```
```

**Step 2: Add usage section**

```markdown
## 使用方法 / Usage

启动 RoboPartner:

```bash
robopartner
```

### 斜杠命令

在对话中直接使用以下命令:

| 命令 | 功能 |
|------|------|
| `/help` | 显示帮助 |
| `/config` | 编辑配置 |
| `/clear` | 清空对话 |
| `/agent list` | 列出 Agents |
| `/browser open` | 打开浏览器 |
| Ctrl+D | 退出 |
```

**Step 3: Commit**

```bash
git add README.md
git commit -m "docs: update README with installation instructions and slash commands"
```

---

## Task 9: 创建 CHANGELOG.md 条目

**Files:**
- Modify: `CHANGELOG.md`

**Step 1: Add v0.2.1 entry**

```markdown
## [0.2.1] - 2025-02-21

### Added / 新增
- **Cross-platform installation scripts** - install.sh (Unix) and install.ps1 (Windows)
  - Automatic dependency detection
  - One-command installation
- **Claude Code-style terminal UI** - Beautiful welcome screen with ASCII art logo
  - Adaptive width borders
  - Colored output (blue theme)
  - Clean message separation
- **Slash command system** - Execute commands directly in chat
  - /help, /config, /clear, /agent, /browser
  - No need for terminal commands

### Changed / 更改
- Refactored InteractiveMode to use new Terminal UI module
- Removed emoji from interface, using [tags] instead
- Improved error messages with colors

### Technical / 技术细节
- New: src/utils/terminal.h/cpp - Terminal and UI utility classes
- Platform detection for Windows/macOS/Linux
- ANSI color code support detection
- Terminal width/height detection
```

**Step 2: Commit**

```bash
git add CHANGELOG.md
git commit -m "docs: add v0.2.1 changelog entry"
```

---

## Task 10: 最终测试和提交

**Files:**
- Test all functionality
- Final commit

**Step 1: Run full build test**

Run: `ninja -C build`

Expected: Clean build with no errors

**Step 2: Run application test**

Run: `./build/robopartner`

Test checklist:
- [ ] Welcome screen displays correctly
- [ ] Colors work
- [ ] Slash commands work
- [ ] Conversation works
- [ ] Exit works (Ctrl+D)

**Step 3: Final commit**

```bash
git add -A
git commit -m "feat: complete terminal UI and installation experience

This commit completes the terminal UI redesign and installation
experience improvements:

Installation:
- Cross-platform install scripts (Unix & Windows)
- Automatic dependency detection and helpful install commands
- One-command installation

Terminal UI:
- Claude Code-style welcome screen with ASCII art
- Adaptive width borders using Terminal class
- Blue color theme throughout
- Clean message separation with colored headers

User Experience:
- Slash commands work directly in chat
- No emoji, clean [tag] format
- Model info display on startup
- Usage tips always visible

Files changed:
- install, install.ps1: Installation scripts
- src/utils/terminal.{h,cpp}: Terminal and UI utilities
- src/cli/interactive_mode.{h,cpp}: Refactored for new UI
- CMakeLists.txt: Added terminal.cpp
- README.md, CHANGELOG.md: Documentation updates"

Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>
```

---

## End of Implementation Plan

**Total estimated time:** 2-3 hours

**Dependencies:** None (uses existing codebase)

**Testing strategy:**
1. Compile after each task
2. Manual UI testing after Task 7
3. Cross-platform testing (if possible)

**Key files to touch:**
- `install` (create)
- `install.ps1` (create)
- `src/utils/terminal.h` (create)
- `src/utils/terminal.cpp` (create)
- `src/cli/interactive_mode.h` (modify)
- `src/cli/interactive_mode.cpp` (modify - major rewrite)
- `CMakeLists.txt` (modify)
- `README.md` (modify)
- `CHANGELOG.md` (modify)
