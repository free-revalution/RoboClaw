// Terminal UI Utility Module Implementation - Terminal UI 工具模块实现

#include "terminal.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cctype>

#ifdef TERMINAL_PLATFORM_UNIX
    #include <sys/ioctl.h>
    #include <unistd.h>
    #include <sys/stat.h>
    #include <fcntl.h>
#endif

#ifdef TERMINAL_PLATFORM_WINDOWS
    #include <windows.h>
    #include <io.h>
    #include <fcntl.h>
#endif

namespace roboclaw {

// Static member initialization
bool Terminal::virtualTerminalEnabled_ = false;

//===========================================================
// Terminal class implementation
//===========================================================

int Terminal::getWidth() {
    #ifdef TERMINAL_PLATFORM_UNIX
        struct winsize w;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
            return w.ws_col;
        }
        return 80;  // Default fallback
    #elif defined(TERMINAL_PLATFORM_WINDOWS)
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hStdOut != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(hStdOut, &csbi)) {
            return csbi.srWindow.Right - csbi.srWindow.Left + 1;
        }
        return 80;
    #else
        return 80;
    #endif
}

int Terminal::getHeight() {
    #ifdef TERMINAL_PLATFORM_UNIX
        struct winsize w;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
            return w.ws_row;
        }
        return 24;  // Default fallback
    #elif defined(TERMINAL_PLATFORM_WINDOWS)
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hStdOut != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(hStdOut, &csbi)) {
            return csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        }
        return 24;
    #else
        return 24;
    #endif
}

void Terminal::clear() {
    #ifdef TERMINAL_PLATFORM_UNIX
        std::cout << "\033[2J\033[H" << std::flush;
    #elif defined(TERMINAL_PLATFORM_WINDOWS)
        // Enable virtual terminal for ANSI support
        enableVirtualTerminal();
        std::cout << "\033[2J\033[H" << std::flush;
    #else
        // Fallback: print many newlines
        for (int i = 0; i < 100; ++i) {
            std::cout << '\n';
        }
    #endif
}

void Terminal::clearLine() {
    #ifdef TERMINAL_PLATFORM_UNIX
        std::cout << "\033[2K\r" << std::flush;
    #elif defined(TERMINAL_PLATFORM_WINDOWS)
        enableVirtualTerminal();
        std::cout << "\033[2K\r" << std::flush;
    #else
        std::cout << "\r" << std::string(80, ' ') << "\r" << std::flush;
    #endif
}

void Terminal::hideCursor() {
    #ifdef TERMINAL_PLATFORM_UNIX
        std::cout << "\033[?25l" << std::flush;
    #elif defined(TERMINAL_PLATFORM_WINDOWS)
        enableVirtualTerminal();
        std::cout << "\033[?25l" << std::flush;
    #endif
}

void Terminal::showCursor() {
    #ifdef TERMINAL_PLATFORM_UNIX
        std::cout << "\033[?25h" << std::flush;
    #elif defined(TERMINAL_PLATFORM_WINDOWS)
        enableVirtualTerminal();
        std::cout << "\033[?25h" << std::flush;
    #endif
}

void Terminal::moveCursor(int row, int col) {
    #ifdef TERMINAL_PLATFORM_UNIX
        std::cout << "\033[" << row << ";" << col << "H" << std::flush;
    #elif defined(TERMINAL_PLATFORM_WINDOWS)
        enableVirtualTerminal();
        std::cout << "\033[" << row << ";" << col << "H" << std::flush;
    #endif
}

void Terminal::saveCursor() {
    #ifdef TERMINAL_PLATFORM_UNIX
        std::cout << "\033[s" << std::flush;
    #elif defined(TERMINAL_PLATFORM_WINDOWS)
        enableVirtualTerminal();
        std::cout << "\033[s" << std::flush;
    #endif
}

void Terminal::restoreCursor() {
    #ifdef TERMINAL_PLATFORM_UNIX
        std::cout << "\033[u" << std::flush;
    #elif defined(TERMINAL_PLATFORM_WINDOWS)
        enableVirtualTerminal();
        std::cout << "\033[u" << std::flush;
    #endif
}

bool Terminal::supportsColor() {
    // Check NO_COLOR environment variable
    if (std::getenv("NO_COLOR") != nullptr) {
        return false;
    }

    #ifdef TERMINAL_PLATFORM_UNIX
        // Check if we're in a terminal
        if (!isatty(STDOUT_FILENO)) {
            return false;
        }

        // Check TERM environment variable
        const char* term = std::getenv("TERM");
        if (term != nullptr && std::string(term) == "dumb") {
            return false;
        }

        return true;
    #elif defined(TERMINAL_PLATFORM_WINDOWS)
        // Windows 10+ supports ANSI colors
        enableVirtualTerminal();
        return virtualTerminalEnabled_;
    #else
        return false;
    #endif
}

bool Terminal::supportsUnicode() {
    #ifdef TERMINAL_PLATFORM_UNIX
        const char* lang = std::getenv("LANG");
        if (lang != nullptr) {
            std::string langStr(lang);
            if (langStr.find("UTF-8") != std::string::npos ||
                langStr.find("utf-8") != std::string::npos ||
                langStr.find("UTF8") != std::string::npos) {
                return true;
            }
        }
        const char* lc_all = std::getenv("LC_ALL");
        if (lc_all != nullptr) {
            std::string lcAllStr(lc_all);
            if (lcAllStr.find("UTF-8") != std::string::npos ||
                lcAllStr.find("utf-8") != std::string::npos) {
                return true;
            }
        }
        return true;  // Most modern Unix terminals support Unicode
    #elif defined(TERMINAL_PLATFORM_WINDOWS)
        // Windows 10+ supports Unicode
        return true;
    #else
        return false;
    #endif
}

int Terminal::getSafeWidth(int minWidth) {
    int width = getWidth();
    return (width >= minWidth) ? width : minWidth;
}

void Terminal::enableVirtualTerminal() {
    #ifdef TERMINAL_PLATFORM_WINDOWS
        if (!virtualTerminalEnabled_) {
            initVirtualTerminal();
        }
    #endif
}

bool Terminal::initVirtualTerminal() {
    #ifdef TERMINAL_PLATFORM_WINDOWS
        HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hStdOut == INVALID_HANDLE_VALUE) {
            return false;
        }

        DWORD mode = 0;
        if (!GetConsoleMode(hStdOut, &mode)) {
            return false;
        }

        // Enable virtual terminal processing
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        if (!SetConsoleMode(hStdOut, mode)) {
            return false;
        }

        // Set UTF-8 code page
        SetConsoleOutputCP(CP_UTF8);

        virtualTerminalEnabled_ = true;
        return true;
    #else
        return true;
    #endif
}

//===========================================================
// UI class implementation
//===========================================================

void UI::drawBox(const std::string& title,
                const std::vector<std::string>& content,
                int width,
                BoxStyle style,
                Alignment alignment) {
    // Auto-calculate width if not specified
    if (width <= 0) {
        width = Terminal::getSafeWidth(40);
        // Limit max width for readability
        if (width > 100) width = 100;
    }

    BoxChars box = getBoxChars(style);
    bool useUnicode = Terminal::supportsUnicode();

    // If no Unicode support, fall back to ASCII
    if (!useUnicode && style != BoxStyle::ASCII) {
        style = BoxStyle::ASCII;
        box = getBoxChars(style);
    }

    int innerWidth = width - 2;  // Width between borders

    // Draw top border
    std::cout << box.topLeft;
    if (!title.empty()) {
        std::string decoratedTitle = " " + title + " ";
        int titleLen = TerminalUtils::visibleLength(decoratedTitle);
        if (titleLen < innerWidth) {
            int leftPadding = (innerWidth - titleLen) / 2;
            for (int i = 0; i < leftPadding; ++i) {
                std::cout << box.horizontal;
            }
            std::cout << Color::TECH_BLUE << decoratedTitle << Color::RESET;
            for (int i = leftPadding + titleLen; i < innerWidth; ++i) {
                std::cout << box.horizontal;
            }
        } else {
            for (int i = 0; i < innerWidth; ++i) {
                std::cout << box.horizontal;
            }
        }
    } else {
        for (int i = 0; i < innerWidth; ++i) {
            std::cout << box.horizontal;
        }
    }
    std::cout << box.topRight << '\n';

    // Draw content
    for (const auto& line : content) {
        std::cout << box.vertical;
        std::string paddedLine = line;

        // Align text
        int lineLen = TerminalUtils::visibleLength(line);
        if (alignment == Alignment::CENTER && lineLen < innerWidth) {
            int leftPad = (innerWidth - lineLen) / 2;
            int rightPad = innerWidth - lineLen - leftPad;
            paddedLine = std::string(leftPad, ' ') + line + std::string(rightPad, ' ');
        } else if (alignment == Alignment::RIGHT && lineLen < innerWidth) {
            paddedLine = std::string(innerWidth - lineLen, ' ') + line;
        } else if (alignment == Alignment::LEFT && lineLen < innerWidth) {
            paddedLine = line + std::string(innerWidth - lineLen, ' ');
        }

        // Truncate if too long
        if (TerminalUtils::visibleLength(paddedLine) > innerWidth) {
            paddedLine = TerminalUtils::truncate(paddedLine, innerWidth);
        }

        std::cout << paddedLine << box.vertical << '\n';
    }

    // Draw bottom border
    std::cout << box.bottomLeft;
    for (int i = 0; i < innerWidth; ++i) {
        std::cout << box.horizontal;
    }
    std::cout << box.bottomRight << '\n';
}

void UI::drawBox(const std::string& title,
                const std::string& content,
                int width,
                BoxStyle style,
                Alignment alignment) {
    std::vector<std::string> lines = wrapText(content, width - 4);
    drawBox(title, lines, width, style, alignment);
}

void UI::drawSeparator(const std::string& style) {
    int width = Terminal::getSafeWidth(40);

    if (style == "single") {
        std::cout << Color::TECH_BLUE;
        for (int i = 0; i < width; ++i) std::cout << "─";
    } else if (style == "double") {
        std::cout << Color::TECH_BLUE;
        for (int i = 0; i < width; ++i) std::cout << "═";
    } else if (style == "dashed") {
        for (int i = 0; i < width; ++i) {
            if (i % 2 == 0) std::cout << "─";
            else std::cout << " ";
        }
    } else if (style == "dotted") {
        for (int i = 0; i < width; ++i) std::cout << "┄";
    } else if (style == "bold") {
        std::cout << Color::BOLD_BLUE;
        for (int i = 0; i < width; ++i) std::cout << "━";
    } else {
        // Default to single
        std::cout << Color::TECH_BLUE;
        for (int i = 0; i < width; ++i) std::cout << "─";
    }
    std::cout << Color::RESET << '\n';
}

void UI::drawLogo() {
    int termWidth = Terminal::getSafeWidth(70);

    bool useColor = Terminal::supportsColor();
    const char* logoColor = useColor ? Color::TECH_BLUE : "";
    const char* reset = useColor ? Color::RESET : "";

    // ASCII Art Logo for RoboClaw - pure text without colors for length calculation
    std::vector<std::string> logoArtPlain = {
        "   ██████╗  ██████╗ ██████╗ ██████╗  ██████╗██╗      █████╗██╗    ██╗",
        "   ██╔══██╗██╔══██╗██╔══██╗██╔══██╗██╔════╝██║     ██╔══██╗██║    ██║",
        "   ██████╔╝██████╔╝██████╔╝██████╔╝██║     ██║     ███████║██║ █╗ ██║",
        "   ██╔══██╗██╔══██╗██╔══██╗██╔══██╗██║     ██║     ██╔══██║██║███╗██║",
        "   ██║  ██║██████╔╝██████╔╝██████╔╝╚██████╗███████╗██║  ██║╚███╔███╔╝",
        "   ╚═╝  ╚═╝╚═════╝ ╚═════╝ ╚═════╝  ╚═════╝╚══════╝╚═╝  ╚═╝ ╚══╝╚══╝"
    };

    // Calculate max line length (pure text, no ANSI codes)
    int maxLineLen = 0;
    for (const auto& line : logoArtPlain) {
        if (static_cast<int>(line.length()) > maxLineLen) {
            maxLineLen = line.length();
        }
    }

    // Box width: border + content + border + 2 spaces padding on each side
    int contentWidth = maxLineLen + 4;  // 2 spaces on each side
    int boxWidth = contentWidth + 2;  // +2 for the two border chars
    if (boxWidth > termWidth - 2) {
        boxWidth = termWidth - 2;
        contentWidth = boxWidth - 2;
    }

    // Draw top border with title
    std::cout << logoColor << "╭─── RoboClaw v1.0.0 ";
    int titleLen = 18;
    int remainingWidth = boxWidth - titleLen - 2;
    for (int i = 0; i < remainingWidth; i++) std::cout << "─";
    std::cout << "╮" << reset << '\n';

    // Draw logo lines with borders
    for (size_t i = 0; i < logoArtPlain.size(); i++) {
        std::cout << logoColor << "│ " << reset << logoColor << logoArtPlain[i] << reset;

        // Calculate padding needed
        int lineLen = logoArtPlain[i].length();
        int padding = contentWidth - lineLen - 2;  // -2 for the initial "│ "
        for (int j = 0; j < padding; j++) std::cout << ' ';

        std::cout << logoColor << " │" << reset << '\n';
    }

    // Draw bottom border
    std::cout << logoColor << "╰";
    for (int i = 0; i < boxWidth - 2; i++) std::cout << "─";
    std::cout << "╯" << reset << '\n';
    std::cout << '\n';
}

void UI::drawModelInfo(const std::string& model, const std::string& provider) {
    int termWidth = Terminal::getSafeWidth(70);
    int boxWidth = termWidth - 4;
    if (boxWidth > 60) boxWidth = 60;

    bool useColor = Terminal::supportsColor();
    const char* boxColor = useColor ? Color::TECH_BLUE : "";
    const char* textColor = useColor ? Color::BOLD_CYAN : "";
    const char* reset = useColor ? Color::RESET : "";

    // Content lines - plain text for length calculation
    std::vector<std::string> contentPlain = {
        " Current Model ",
        "",
        " Model:    " + model,
        " Provider: " + provider
    };

    // Draw top border
    std::cout << boxColor << "╭";
    for (int i = 0; i < boxWidth - 2; i++) std::cout << "─";
    std::cout << "╮" << reset << '\n';

    // Draw content
    int innerWidth = boxWidth - 4;  // Width inside borders
    for (size_t i = 0; i < contentPlain.size(); i++) {
        std::cout << boxColor << "│ " << reset;

        // First line gets special color treatment
        if (i == 0) {
            std::cout << textColor << contentPlain[i] << reset;
        } else if (i == 1) {
            // Empty line
        } else if (i == 2) {
            // Model line - color the label
            size_t colonPos = contentPlain[i].find(':');
            std::cout << textColor << contentPlain[i].substr(0, colonPos + 1) << reset
                      << contentPlain[i].substr(colonPos + 1);
        } else if (i == 3) {
            // Provider line - color the label
            size_t colonPos = contentPlain[i].find(':');
            std::cout << textColor << contentPlain[i].substr(0, colonPos + 1) << reset
                      << contentPlain[i].substr(colonPos + 1);
        }

        int lineLen = contentPlain[i].length();
        int padding = innerWidth - lineLen - 1;  // -1 for the leading space after border
        for (int j = 0; j < padding; j++) std::cout << ' ';
        std::cout << boxColor << " │" << reset << '\n';
    }

    // Draw bottom border
    std::cout << boxColor << "╰";
    for (int i = 0; i < boxWidth - 2; i++) std::cout << "─";
    std::cout << "╯" << reset << '\n';
    std::cout << '\n';
}

void UI::drawUsageTips() {
    std::cout << Color::BOLD_YELLOW << "\nTips / 提示:" << Color::RESET << '\n';
    std::cout << "  • Type " << Color::BOLD_GREEN << "/help" << Color::RESET
              << " for commands / 输入 /help 查看命令\n";
    std::cout << "  • " << Color::BOLD_GREEN << "Ctrl+D" << Color::RESET
              << " to exit / Ctrl+D 退出\n";
    std::cout << "  • Type " << Color::BOLD_GREEN << "/config" << Color::RESET
              << " to change settings / /config 修改配置\n";
    std::cout << "  • Type " << Color::BOLD_GREEN << "/session" << Color::RESET
              << " for session management / /config 会话管理\n";
    std::cout << '\n';
}

void UI::drawProgressBar(int current, int total, int width, const std::string& label) {
    if (total <= 0) total = 1;
    if (current > total) current = total;
    if (current < 0) current = 0;

    float percentage = static_cast<float>(current) / total;
    int filled = static_cast<int>(percentage * width);
    if (filled < 0) filled = 0;
    if (filled > width) filled = width;

    if (!label.empty()) {
        std::cout << label << " ";
    }

    std::cout << Color::TECH_BLUE << "[";
    for (int i = 0; i < width; ++i) {
        if (i < filled) {
            std::cout << Color::BOLD_GREEN << "█" << Color::TECH_BLUE;
        } else {
            std::cout << "░";
        }
    }
    std::cout << "]" << Color::RESET;

    std::cout << " " << std::setw(3) << static_cast<int>(percentage * 100) << "%\r";
    std::cout << std::flush;
}

void UI::drawSpinner(const std::string& message) {
    static const char* frames[] = {"⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"};
    static int frameIndex = 0;

    std::cout << "\r" << Color::TECH_CYAN << frames[frameIndex] << Color::RESET
              << " " << message << "    \r" << std::flush;
    frameIndex = (frameIndex + 1) % 10;
}

void UI::drawError(const std::string& message) {
    std::vector<std::string> content = {message};
    drawBox(std::string(Color::BOLD_RED) + " ✗ Error / 错误" + Color::RESET, content, 0, BoxStyle::ROUNDED, Alignment::LEFT);
    std::cout << '\n';
}

void UI::drawWarning(const std::string& message) {
    std::vector<std::string> content = {message};
    drawBox(std::string(Color::BOLD_YELLOW) + " ⚠ Warning / 警告" + Color::RESET, content, 0, BoxStyle::ROUNDED, Alignment::LEFT);
    std::cout << '\n';
}

void UI::drawSuccess(const std::string& message) {
    std::vector<std::string> content = {message};
    drawBox(std::string(Color::BOLD_GREEN) + " ✓ Success / 成功" + Color::RESET, content, 0, BoxStyle::ROUNDED, Alignment::LEFT);
    std::cout << '\n';
}

void UI::drawInfo(const std::string& message) {
    std::vector<std::string> content = {message};
    drawBox(std::string(Color::BOLD_BLUE) + " ℹ Info / 信息" + Color::RESET, content, 0, BoxStyle::ROUNDED, Alignment::LEFT);
    std::cout << '\n';
}

void UI::drawTable(const std::vector<std::vector<std::string>>& data,
                  const std::vector<std::string>& headers) {
    if (data.empty() && headers.empty()) {
        return;
    }

    std::vector<int> colWidths = calculateColumnWidths(data, headers);

    bool useColor = Terminal::supportsColor();

    // Draw top border
    std::cout << Color::TECH_BLUE << "┌";
    for (size_t i = 0; i < colWidths.size(); ++i) {
        for (int j = 0; j < colWidths[i] + 2; ++j) std::cout << "─";
        if (i < colWidths.size() - 1) std::cout << "┬";
    }
    std::cout << "┐" << Color::RESET << '\n';

    // Draw headers
    if (!headers.empty()) {
        std::cout << Color::TECH_BLUE << "│" << Color::RESET;
        for (size_t i = 0; i < headers.size() && i < colWidths.size(); ++i) {
            if (useColor) std::cout << Color::BOLD_CYAN;
            std::cout << " " << std::setw(colWidths[i]) << std::left << headers[i];
            if (useColor) std::cout << Color::RESET;
            std::cout << " " << Color::TECH_BLUE << "│" << Color::RESET;
        }
        std::cout << '\n';

        // Draw separator after headers
        std::cout << Color::TECH_BLUE << "├";
        for (size_t i = 0; i < colWidths.size(); ++i) {
            for (int j = 0; j < colWidths[i] + 2; ++j) std::cout << "─";
            if (i < colWidths.size() - 1) std::cout << "┼";
        }
        std::cout << "┤" << Color::RESET << '\n';
    }

    // Draw data rows
    for (const auto& row : data) {
        std::cout << Color::TECH_BLUE << "│" << Color::RESET;
        for (size_t i = 0; i < row.size() && i < colWidths.size(); ++i) {
            std::cout << " " << std::setw(colWidths[i]) << std::left << row[i];
            std::cout << " " << Color::TECH_BLUE << "│" << Color::RESET;
        }
        std::cout << '\n';
    }

    // Draw bottom border
    std::cout << Color::TECH_BLUE << "└";
    for (size_t i = 0; i < colWidths.size(); ++i) {
        for (int j = 0; j < colWidths[i] + 2; ++j) std::cout << "─";
        if (i < colWidths.size() - 1) std::cout << "┴";
    }
    std::cout << "┘" << Color::RESET << '\n';
}

void UI::drawRule(const std::string& text) {
    int width = Terminal::getSafeWidth(40);

    if (text.empty()) {
        std::cout << Color::TECH_BLUE;
        for (int i = 0; i < width; ++i) std::cout << "─";
        std::cout << Color::RESET << '\n';
        return;
    }

    int textLen = TerminalUtils::visibleLength(text);
    if (textLen >= width - 4) {
        std::cout << text << '\n';
        return;
    }

    int sideWidth = (width - textLen - 2) / 2;

    std::cout << Color::TECH_BLUE;
    for (int i = 0; i < sideWidth; ++i) std::cout << "─";
    std::cout << Color::RESET << " " << text << " " << Color::TECH_BLUE;
    for (int i = 0; i < width - sideWidth - textLen - 2; ++i) std::cout << "─";
    std::cout << Color::RESET << '\n';
}

void UI::drawKeyValue(const std::string& key, const std::string& value, int keyWidth) {
    std::cout << Color::BOLD_CYAN << std::setw(keyWidth) << std::left << key
              << Color::RESET << ": " << value << '\n';
}

std::vector<std::string> UI::wrapText(const std::string& text, int width) {
    std::vector<std::string> lines;
    std::string current;
    int currentLen = 0;

    std::istringstream iss(text);
    std::string word;

    while (iss >> word) {
        int wordLen = TerminalUtils::visibleLength(word);

        if (currentLen == 0) {
            current = word;
            currentLen = wordLen;
        } else if (currentLen + 1 + wordLen <= width) {
            current += " " + word;
            currentLen += 1 + wordLen;
        } else {
            lines.push_back(current);
            current = word;
            currentLen = wordLen;
        }
    }

    if (!current.empty()) {
        lines.push_back(current);
    }

    return lines;
}

UI::BoxChars UI::getBoxChars(BoxStyle style) {
    switch (style) {
        case BoxStyle::DOUBLE:
            return {"╔", "╗", "╚", "╝", "═", "║", "╠", "╣", "╦", "╩", "╬"};
        case BoxStyle::ROUNDED:
            return {"╭", "╮", "╰", "╯", "─", "│", "├", "┤", "┬", "┴", "┼"};
        case BoxStyle::ASCII:
            return {"+", "+", "+", "+", "-", "|", "+", "+", "+", "+", "+"};
        case BoxStyle::BOLD:
            return {"┏", "┓", "┗", "┛", "━", "┃", "┣", "┫", "┳", "┻", "╋"};
        case BoxStyle::SINGLE:
        default:
            return {"┌", "┐", "└", "┘", "─", "│", "├", "┤", "┬", "┴", "┼"};
    }
}

std::string UI::centerString(const std::string& str, int width) {
    int len = TerminalUtils::visibleLength(str);
    if (len >= width) return str;
    int padding = (width - len) / 2;
    return std::string(padding, ' ') + str;
}

std::vector<int> UI::calculateColumnWidths(
    const std::vector<std::vector<std::string>>& data,
    const std::vector<std::string>& headers) {

    size_t numCols = headers.size();
    if (!data.empty() && data[0].size() > numCols) {
        numCols = data[0].size();
    }

    std::vector<int> colWidths(numCols, 0);

    // Check headers
    for (size_t i = 0; i < headers.size(); ++i) {
        int len = TerminalUtils::visibleLength(headers[i]);
        if (len > colWidths[i]) colWidths[i] = len;
    }

    // Check data
    for (const auto& row : data) {
        for (size_t i = 0; i < row.size() && i < numCols; ++i) {
            int len = TerminalUtils::visibleLength(row[i]);
            if (len > colWidths[i]) colWidths[i] = len;
        }
    }

    return colWidths;
}

//===========================================================
// Spinner class implementation
//===========================================================

const char* Spinner::FRAMES[] = {"⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"};

Spinner::Spinner(const std::string& message)
    : message_(message), frame_(0), stopped_(false) {
    Terminal::hideCursor();
}

Spinner::~Spinner() {
    stop();
}

void Spinner::update() {
    if (stopped_) return;

    std::cout << "\r" << Color::TECH_CYAN << FRAMES[frame_] << Color::RESET
              << " " << message_ << "    \r" << std::flush;

    frame_ = (frame_ + 1) % 10;
}

void Spinner::setMessage(const std::string& message) {
    message_ = message;
}

void Spinner::stop() {
    if (stopped_) return;
    stopped_ = true;

    // Clear the spinner line
    Terminal::clearLine();
    Terminal::showCursor();
}

//===========================================================
// TerminalUtils implementation
//===========================================================

namespace TerminalUtils {

std::string truncate(const std::string& str, int maxLength) {
    if (maxLength <= 0) return "";

    int visibleLen = visibleLength(str);
    if (visibleLen <= maxLength) return str;

    // Simple truncation (doesn't handle multi-byte Unicode properly)
    // For production, should use proper Unicode-aware truncation
    if (str.length() <= static_cast<size_t>(maxLength)) {
        return str;
    }

    return str.substr(0, maxLength - 3) + "...";
}

std::string repeat(const std::string& str, int count) {
    if (count <= 0) return "";
    std::string result;
    result.reserve(str.length() * count);
    for (int i = 0; i < count; ++i) {
        result += str;
    }
    return result;
}

std::string stripAnsi(const std::string& str) {
    std::string result;
    bool inEscape = false;

    for (char c : str) {
        if (c == '\033') {
            inEscape = true;
        } else if (inEscape && c == 'm') {
            inEscape = false;
        } else if (!inEscape) {
            result += c;
        }
    }

    return result;
}

int visibleLength(const std::string& str) {
    return stripAnsi(str).length();
}

bool isTTY() {
    #ifdef TERMINAL_PLATFORM_UNIX
        return isatty(STDOUT_FILENO);
    #elif defined(TERMINAL_PLATFORM_WINDOWS)
        HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode = 0;
        return GetConsoleMode(hStdOut, &mode) != 0;
    #else
        return false;
    #endif
}

} // namespace TerminalUtils

} // namespace roboclaw
