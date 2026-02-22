// Terminal UI Utility Module - Terminal UI 工具模块
// Provides cross-platform terminal UI utilities - 提供跨平台终端UI工具

#ifndef ROBOCLAW_UTILS_TERMINAL_H
#define ROBOCLAW_UTILS_TERMINAL_H

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

// Platform detection for terminal utilities
#if defined(PLATFORM_WINDOWS) || defined(_WIN32) || defined(_WIN64)
    #define TERMINAL_PLATFORM_WINDOWS
#elif defined(PLATFORM_MACOS) || defined(__APPLE__) || defined(PLATFORM_LINUX) || defined(__linux__)
    #define TERMINAL_PLATFORM_UNIX
#endif

namespace roboclaw {

// ANSI Color definitions namespace
// ANSI颜色定义命名空间
namespace Color {
    // Standard colors - 标准颜色
    const char* const RESET     = "\033[0m";
    const char* const RED       = "\033[0;31m";
    const char* const GREEN     = "\033[0;32m";
    const char* const YELLOW    = "\033[0;33m";
    const char* const BLUE      = "\033[0;34m";
    const char* const MAGENTA   = "\033[0;35m";
    const char* const CYAN      = "\033[0;36m";
    const char* const WHITE     = "\033[0;37m";
    const char* const GRAY      = "\033[0;90m";

    // Bold colors - 粗体颜色
    const char* const BOLD_RED     = "\033[1;31m";
    const char* const BOLD_GREEN   = "\033[1;32m";
    const char* const BOLD_YELLOW  = "\033[1;33m";
    const char* const BOLD_BLUE    = "\033[1;34m";
    const char* const BOLD_MAGENTA = "\033[1;35m";
    const char* const BOLD_CYAN    = "\033[1;36m";
    const char* const BOLD_WHITE   = "\033[1;37m";

    // Background colors - 背景颜色
    const char* const BG_RED       = "\033[41m";
    const char* const BG_GREEN     = "\033[42m";
    const char* const BG_YELLOW    = "\033[43m";
    const char* const BG_BLUE      = "\033[44m";
    const char* const BG_MAGENTA   = "\033[45m";
    const char* const BG_CYAN      = "\033[46m";
    const char* const BG_WHITE     = "\033[47m";

    // Tech blue for RoboPartner branding
    const char* const TECH_BLUE    = "\033[38;5;33m";     // Deep tech blue
    const char* const TECH_CYAN    = "\033[38;5;39m";     // Bright cyan
    const char* const TECH_PURPLE  = "\033[38;5;57m";     // Purple accent

    // Platform-specific empty string for Windows without ANSI support
    #ifdef TERMINAL_PLATFORM_WINDOWS
    inline const char* ansiOrEmpty(const char* ansiCode) {
        // Check if we should use ANSI (Windows 10+ supports it)
        return ansiCode;  // Modern Windows supports ANSI
    }
    #else
    inline const char* ansiOrEmpty(const char* ansiCode) {
        return ansiCode;
    }
    #endif
}

// Terminal class - handles terminal detection and control
// Terminal类 - 处理终端检测和控制
class Terminal {
public:
    // Get terminal width in columns - 获取终端宽度（列数）
    static int getWidth();

    // Get terminal height in rows - 获取终端高度（行数）
    static int getHeight();

    // Clear the screen - 清屏
    static void clear();

    // Clear current line - 清除当前行
    static void clearLine();

    // Hide cursor - 隐藏光标
    static void hideCursor();

    // Show cursor - 显示光标
    static void showCursor();

    // Move cursor to position (1-indexed) - 移动光标到指定位置（从1开始）
    static void moveCursor(int row, int col);

    // Save cursor position - 保存光标位置
    static void saveCursor();

    // Restore cursor position - 恢复光标位置
    static void restoreCursor();

    // Check if terminal supports colors - 检查终端是否支持颜色
    static bool supportsColor();

    // Check if terminal supports Unicode - 检查终端是否支持Unicode
    static bool supportsUnicode();

    // Get safe width (with minimum limit) - 获取安全宽度（带最小限制）
    static int getSafeWidth(int minWidth = 40);

    // Enable virtual terminal processing on Windows (for ANSI support)
    // 在Windows上启用虚拟终端处理（支持ANSI）
    static void enableVirtualTerminal();

private:
    // Check if virtual terminal is enabled
    static bool virtualTerminalEnabled_;
    static bool initVirtualTerminal();
};

// UI class - high-level UI drawing functions
// UI类 - 高级UI绘制函数
class UI {
public:
    // Box alignment options - 盒子对齐选项
    enum class Alignment {
        LEFT,
        CENTER,
        RIGHT
    };

    // Box style options - 盒子样式选项
    enum class BoxStyle {
        SINGLE,     // ╔═╗║╚╝
        DOUBLE,     // ╔═╗║╚╝ (double lines)
        ROUNDED,    // ╭─╮│╰╯
        ASCII,      // +-||  (plain ASCII)
        BOLD        // ┏━┓┃┗┛
    };

    // Draw a bordered box with content - 绘制带边框的盒子
    // title: Optional title text - 可选标题文本
    // content: Vector of content lines - 内容行向量
    // width: Box width (0 for auto) - 盒子宽度（0为自动）
    // style: Box drawing style - 盒子绘制样式
    // alignment: Text alignment inside box - 盒子内文本对齐方式
    static void drawBox(const std::string& title,
                       const std::vector<std::string>& content,
                       int width = 0,
                       BoxStyle style = BoxStyle::SINGLE,
                       Alignment alignment = Alignment::LEFT);

    // Draw a box with single string content (auto-wrapped)
    // 绘制包含单个字符串内容的盒子（自动换行）
    static void drawBox(const std::string& title,
                       const std::string& content,
                       int width = 0,
                       BoxStyle style = BoxStyle::SINGLE,
                       Alignment alignment = Alignment::LEFT);

    // Draw a separator line - 绘制分隔线
    // style: "single", "double", "dashed", "dotted", "bold"
    static void drawSeparator(const std::string& style = "single");

    // Draw the RoboPartner ASCII art logo - 绘制RoboPartner ASCII艺术标志
    static void drawLogo();

    // Draw model information box - 绘制模型信息框
    static void drawModelInfo(const std::string& model, const std::string& provider);

    // Draw usage tips - 绘制使用提示
    static void drawUsageTips();

    // Draw a progress bar - 绘制进度条
    static void drawProgressBar(int current, int total, int width = 40,
                               const std::string& label = "");

    // Draw a spinning loader animation - 绘制旋转加载动画
    static void drawSpinner(const std::string& message);

    // Draw error message box - 绘制错误消息框
    static void drawError(const std::string& message);

    // Draw warning message box - 绘制警告消息框
    static void drawWarning(const std::string& message);

    // Draw success message box - 绘制成功消息框
    static void drawSuccess(const std::string& message);

    // Draw info message box - 绘制信息消息框
    static void drawInfo(const std::string& message);

    // Draw a table - 绘制表格
    static void drawTable(const std::vector<std::vector<std::string>>& data,
                         const std::vector<std::string>& headers = {});

    // Draw a horizontal rule with text - 绘制带文本的水平线
    static void drawRule(const std::string& text = "");

    // Draw a key-value pair - 绘制键值对
    static void drawKeyValue(const std::string& key, const std::string& value,
                            int keyWidth = 20);

private:
    // Word wrap text to fit width - 文本自动换行以适应宽度
    static std::vector<std::string> wrapText(const std::string& text, int width);

    // Get box drawing characters - 获取盒子绘制字符
    struct BoxChars {
        const char* topLeft;
        const char* topRight;
        const char* bottomLeft;
        const char* bottomRight;
        const char* horizontal;
        const char* vertical;
        const char* leftTee;
        const char* rightTee;
        const char* topTee;
        const char* bottomTee;
        const char* cross;
    };

    static BoxChars getBoxChars(BoxStyle style);

    // Pad string to center - 填充字符串以居中
    static std::string centerString(const std::string& str, int width);

    // Calculate column widths for table - 计算表格列宽
    static std::vector<int> calculateColumnWidths(
        const std::vector<std::vector<std::string>>& data,
        const std::vector<std::string>& headers);
};

// Spinner animation class - 旋转动画类
class Spinner {
public:
    Spinner(const std::string& message = "");
    ~Spinner();

    // Update the spinner frame - 更新旋转动画帧
    void update();

    // Set message - 设置消息
    void setMessage(const std::string& message);

    // Stop and remove spinner - 停止并移除旋转动画
    void stop();

private:
    std::string message_;
    int frame_;
    bool stopped_;
    static const char* FRAMES[];
};

// Utility functions - 工具函数
namespace TerminalUtils {
    // Truncate string with ellipsis - 用省略号截断字符串
    std::string truncate(const std::string& str, int maxLength);

    // Repeat string - 重复字符串
    std::string repeat(const std::string& str, int count);

    // Strip ANSI codes from string - 从字符串中移除ANSI码
    std::string stripAnsi(const std::string& str);

    // Get visible string length (without ANSI codes) - 获取可见字符串长度（不含ANSI码）
    int visibleLength(const std::string& str);

    // Detect if terminal is a TTY - 检测终端是否为TTY
    bool isTTY();
}

} // namespace roboclaw

#endif // ROBOCLAW_UTILS_TERMINAL_H
