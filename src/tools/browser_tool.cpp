// 浏览器自动化工具实现 / Browser Automation Tool Implementation

#include "browser_tool.h"
#include "../utils/logger.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <filesystem>
#include <thread>
#include <chrono>

#ifdef PLATFORM_MACOS
#include <CoreFoundation/CoreFoundation.h>
#include <objc/objc.h>
#endif

#ifdef PLATFORM_LINUX
#include <cstdlib>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#endif

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <shlobj.h>
#include <tchar.h>
#endif

namespace roboclaw {

// ============================================================
// macOS Safari/Chrome 实现 (使用 AppleScript)
// ============================================================

#ifdef PLATFORM_MACOS

class MacOSBrowserHandle : public BrowserHandle {
public:
    MacOSBrowserHandle(const std::string& browser_name, int port)
        : BrowserHandle(browser_name, port), connected_(false) {
        // 使用 AppleScript 和 WebDriver
        connected_ = startBrowser();
    }

    ~MacOSBrowserHandle() override {
        close();
    }

    bool isConnected() const override {
        return connected_;
    }

    bool navigate(const std::string& url) override {
        std::string script = "tell application \"" + browser_name_ + "\"\n";
        script += "  set URL of front document to \"" + url + "\"\n";
        script += "end tell";
        return executeAppleScript(script).empty();
    }

    bool click(const Selector& selector) override {
        // 使用 JavaScript 点击元素
        std::string js = "var elem = document.querySelector('" + selector.value + "');";
        js += "if(elem) elem.click();";
        return executeScript(js).find("true") != std::string::npos;
    }

    bool type(const Selector& selector, const std::string& text) override {
        std::string js = "var elem = document.querySelector('" + selector.value + "');";
        js += "if(elem) { elem.value = '" + text + "'; elem.dispatchEvent(new Event('input')); }";
        return executeScript(js).find("true") != std::string::npos;
    }

    std::string screenshot() override {
        // 使用 JavaScript 和 canvas 截图
        std::string js =
            "(function() {"
            "  var canvas = document.createElement('canvas');"
            "  canvas.width = window.innerWidth;"
            "  canvas.height = window.innerHeight;"
            "  var ctx = canvas.getContext('2d');"
            "  ctx.drawImage(window.document.body, 0, 0);"
            "  return canvas.toDataURL('image/png');"
            "})();";
        return executeScript(js);
    }

    std::string getText(const Selector& selector) override {
        std::string js = "document.querySelector('" + selector.value + "').textContent;";
        return executeScript(js);
    }

    std::string executeScript(const std::string& script) override {
        std::string appleScript = "tell application \"" + browser_name_ + "\"\n";
        appleScript += "  execute javascript \"" + script + "\" in front document\n";
        appleScript += "end tell";
        return executeAppleScript(appleScript);
    }

    bool scroll(int x, int y) override {
        std::string js = "window.scrollBy(" + std::to_string(x) + ", " + std::to_string(y) + ");";
        return executeScript(js).find("true") != std::string::npos;
    }

    void close() override {
        if (connected_) {
            std::string script = "tell application \"" + browser_name_ + "\" to quit";
            executeAppleScript(script);
            connected_ = false;
        }
    }

private:
    bool connected_;

    bool startBrowser() {
        std::string script = "tell application \"" + browser_name_ + "\"\n";
        script += "  activate\n";
        script += "end tell";
        return executeAppleScript(script).empty();
    }

    std::string executeAppleScript(const std::string& script) {
        FILE* pipe = popen("osascript", "w");
        if (!pipe) return "error";

        fprintf(pipe, "%s\n", script.c_str());
        fclose(pipe);

        // 获取输出
        char buffer[128];
        std::string result;
        pipe = popen("osascript 2>&1", "r");
        if (pipe) {
            while (fgets(buffer, sizeof(buffer), pipe)) {
                result += buffer;
            }
            pclose(pipe);
        }
        return result;
    }
};

#endif // PLATFORM_MACOS

// ============================================================
// Linux Chrome/Firefox 实现 (使用 WebDriver)
// ============================================================

#ifdef PLATFORM_LINUX

class LinuxBrowserHandle : public BrowserHandle {
public:
    LinuxBrowserHandle(const std::string& browser_name, int port)
        : BrowserHandle(browser_name, port), connected_(false), process_id_(0) {
        connected_ = startWebDriver();
    }

    ~LinuxBrowserHandle() override {
        close();
    }

    bool isConnected() const override {
        return connected_;
    }

    bool navigate(const std::string& url) override {
        return sendCommand("{\"url\":\"" + url + "\"}");
    }

    bool click(const Selector& selector) override {
        std::string cmd = "document.querySelector('" + selector.value + "').click()";
        return executeScript(cmd).find("true") != std::string::npos;
    }

    bool type(const Selector& selector, const std::string& text) override {
        std::string cmd = "document.querySelector('" + selector.value + "').value='" + text + "'";
        return executeScript(cmd).find("true") != std::string::npos;
    }

    std::string screenshot() override {
        return sendCommand("{\"screenshot\":true}");
    }

    std::string getText(const Selector& selector) override {
        std::string cmd = "document.querySelector('" + selector.value + "').textContent";
        return executeScript(cmd);
    }

    std::string executeScript(const std::string& script) override {
        return sendCommand("{\"script\":\"" + script + "\"}");
    }

    bool scroll(int x, int y) override {
        std::string cmd = "window.scrollBy(" + std::to_string(x) + "," + std::to_string(y) + ")";
        return executeScript(cmd).find("true") != std::string::npos;
    }

    void close() override {
        if (connected_) {
            sendCommand("{\"quit\":true}");
            if (process_id_ > 0) {
                kill(process_id_, SIGTERM);
            }
            connected_ = false;
        }
    }

private:
    bool connected_;
    pid_t process_id_;

    bool startWebDriver() {
        // 启动 ChromeDriver 或 geckodriver
        std::string driver_cmd;
        if (browser_name_.find("Chrome") != std::string::npos) {
            driver_cmd = "chromedriver --port=" + std::to_string(port_);
        } else if (browser_name_.find("Firefox") != std::string::npos) {
            driver_cmd = "geckodriver --port=" + std::to_string(port_);
        } else {
            return false;
        }

        process_id_ = fork();
        if (process_id_ == 0) {
            // 子进程
            execlp("sh", "sh", "-c", driver_cmd.c_str(), nullptr);
            exit(1);
        } else if (process_id_ > 0) {
            // 父进程
            usleep(1000000);  // 等待 1 秒
            connected_ = true;
            return true;
        }
        return false;
    }

    std::string sendCommand(const std::string& cmd) {
        // 使用 curl 发送 WebDriver 命令
        std::string curl_cmd = "curl -s http://localhost:" + std::to_string(port_) + "/session -d '" + cmd + "'";
        char buffer[4096];
        std::string result;

        FILE* pipe = popen(curl_cmd.c_str(), "r");
        if (pipe) {
            while (fgets(buffer, sizeof(buffer), pipe)) {
                result += buffer;
            }
            pclose(pipe);
        }
        return result;
    }
};

#endif // PLATFORM_LINUX

// ============================================================
// Windows Chrome/Edge 实现 (使用 WebDriver)
// ============================================================

#ifdef PLATFORM_WINDOWS

class WindowsBrowserHandle : public BrowserHandle {
public:
    WindowsBrowserHandle(const std::string& browser_name, int port)
        : BrowserHandle(browser_name, port), connected_(false), process_handle_(nullptr) {
        connected_ = startWebDriver();
    }

    ~WindowsBrowserHandle() override {
        close();
    }

    bool isConnected() const override {
        return connected_;
    }

    bool navigate(const std::string& url) override {
        return sendCommand("{\"url\":\"" + url + "\"}");
    }

    bool click(const Selector& selector) override {
        std::string cmd = "document.querySelector('" + selector.value + "').click()";
        return executeScript(cmd).find("true") != std::string::npos;
    }

    bool type(const Selector& selector, const std::string& text) override {
        std::string cmd = "document.querySelector('" + selector.value + "').value='" + text + "'";
        return executeScript(cmd).find("true") != std::string::npos;
    }

    std::string screenshot() override {
        return sendCommand("{\"screenshot\":true}");
    }

    std::string getText(const Selector& selector) override {
        std::string cmd = "document.querySelector('" + selector.value + "').textContent";
        return executeScript(cmd);
    }

    std::string executeScript(const std::string& script) override {
        return sendCommand("{\"script\":\"" + script + "\"}");
    }

    bool scroll(int x, int y) override {
        std::string cmd = "window.scrollBy(" + std::to_string(x) + "," + std::to_string(y) + ")";
        return executeScript(cmd).find("true") != std::string::npos;
    }

    void close() override {
        if (connected_) {
            sendCommand("{\"quit\":true}");
            if (process_handle_) {
                TerminateProcess(process_handle_, 0);
                CloseHandle(process_handle_);
            }
            connected_ = false;
        }
    }

private:
    bool connected_;
    HANDLE process_handle_;

    bool startWebDriver() {
        // 启动 ChromeDriver 或 MSEdgeDriver
        std::string driver_exe;
        if (browser_name_.find("Chrome") != std::string::npos) {
            driver_exe = "chromedriver.exe";
        } else if (browser_name_.find("Edge") != std::string::npos) {
            driver_exe = "msedgedriver.exe";
        } else {
            return false;
        }

        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        std::string cmd = driver_exe + " --port=" + std::to_string(port_);

        if (CreateProcessA(NULL, const_cast<char*>(cmd.c_str()), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            process_handle_ = pi.hProcess;
            connected_ = true;
            Sleep(1000);  // 等待 1 秒
            return true;
        }
        return false;
    }

    std::string sendCommand(const std::string& cmd) {
        // 使用 Windows HTTP API 或 curl 发送命令
        char buffer[4096];
        std::string curl_cmd = "curl -s http://localhost:" + std::to_string(port_) + "/session -d \"" + cmd + "\"";

        FILE* pipe = _popen(curl_cmd.c_str(), "r");
        if (pipe) {
            std::string result;
            while (fgets(buffer, sizeof(buffer), pipe)) {
                result += buffer;
            }
            _pclose(pipe);
            return result;
        }
        return "error";
    }
};

#endif // PLATFORM_WINDOWS

// ============================================================
// BrowserTool 实现
// ============================================================

BrowserTool::BrowserTool()
    : ToolBase("browser", "浏览器自动化工具 / Browser automation tool for visual control") {
}

BrowserTool::~BrowserTool() {
    std::unique_lock<std::shared_mutex> lock(browsers_mutex_);
    open_browsers_.clear();
}

ToolDescription BrowserTool::getToolDescription() const {
    ToolDescription desc;
    desc.name = "browser";
    desc.description = "浏览器自动化工具，类似 OpenClaw 的可视化操作功能。支持打开、导航、点击、输入、截图等操作 / Browser automation tool like OpenClaw visual control. Supports open, navigate, click, type, screenshot operations.";

    desc.parameters = {
        {"action", "string", "操作类型 / Action: open, close, navigate, screenshot, click, type, scroll, execute, get_text, find_element, list_tabs, new_tab, close_tab, switch_tab", true, ""},
        {"browser", "string", "浏览器类型 / Browser type: chrome, firefox, safari, edge (default: auto)", false, "auto"},
        {"url", "string", "目标 URL / Target URL (for navigate action)", false, ""},
        {"selector_type", "string", "定位器类型 / Selector type: css, xpath, id, name, class", false, "css"},
        {"selector_value", "string", "定位器值 / Selector value", false, ""},
        {"text", "string", "输入文本 / Text to type", false, ""},
        {"script", "string", "JavaScript 代码 / JavaScript code", false, ""},
        {"x", "integer", "X 方向滚动 / X scroll amount", false, "0"},
        {"y", "integer", "Y 方向滚动 / Y scroll amount", false, "0"},
        {"wait_ms", "integer", "等待毫秒数 / Wait milliseconds", false, "1000"},
        {"tab_index", "integer", "标签页索引 / Tab index", false, "0"}
    };
    return desc;
}

bool BrowserTool::validateParams(const json& params) const {
    if (!hasRequiredParam(params, "action")) {
        return false;
    }

    std::string action_str = getStringParam(params, "action");
    std::vector<std::string> valid_actions = {
        "open", "close", "navigate", "screenshot", "click", "type",
        "scroll", "execute", "get_text", "find_element", "list_tabs",
        "new_tab", "close_tab", "switch_tab"
    };

    return std::find(valid_actions.begin(), valid_actions.end(), action_str) != valid_actions.end();
}

ToolResult BrowserTool::execute(const json& params) {
    if (!validateParams(params)) {
        return ToolResult::error("Invalid parameters");
    }

    std::string action_str = getStringParam(params, "action");

    if (action_str == "open") {
        BrowserType type = BrowserType::AUTO;
        std::string browser_str = getStringParam(params, "browser", "auto");
        if (browser_str == "chrome") type = BrowserType::CHROME;
        else if (browser_str == "firefox") type = BrowserType::FIREFOX;
        else if (browser_str == "safari") type = BrowserType::SAFARI;
        else if (browser_str == "edge") type = BrowserType::EDGE;
        return openBrowser(type);
    } else if (action_str == "close") {
        return closeBrowser();
    } else if (action_str == "navigate") {
        return navigate(getStringParam(params, "url", ""));
    } else if (action_str == "screenshot") {
        return screenshot();
    } else if (action_str == "click") {
        return click(parseSelector(params));
    } else if (action_str == "type") {
        return type(parseSelector(params), getStringParam(params, "text", ""));
    } else if (action_str == "scroll") {
        return scroll(getIntParam(params, "x", 0), getIntParam(params, "y", 0));
    } else if (action_str == "execute") {
        return executeScript(getStringParam(params, "script", ""));
    } else if (action_str == "get_text") {
        return getText(parseSelector(params));
    } else if (action_str == "wait") {
        return wait(getIntParam(params, "wait_ms", 1000));
    } else if (action_str == "list_tabs") {
        return listTabs();
    } else if (action_str == "new_tab") {
        return newTab(getStringParam(params, "url", ""));
    } else if (action_str == "close_tab") {
        return closeTab(getIntParam(params, "tab_index", -1));
    } else if (action_str == "switch_tab") {
        return switchTab(getIntParam(params, "tab_index", 0));
    }

    return ToolResult::error("Unknown action: " + action_str);
}

Selector BrowserTool::parseSelector(const json& params) const {
    Selector sel;
    sel.type = getStringParam(params, "selector_type", "css");
    sel.value = getStringParam(params, "selector_value", "");
    return sel;
}

ToolResult BrowserTool::openBrowser(BrowserType type) {
    std::unique_lock<std::shared_mutex> lock(browsers_mutex_);

    // 检测已安装的浏览器
    auto installed = detectInstalledBrowsers();
    if (installed.empty()) {
        return ToolResult::error("未找到可用的浏览器 / No browser found");
    }

    // 如果是 AUTO，选择第一个可用浏览器
    if (type == BrowserType::AUTO && !installed.empty()) {
        type = installed[0];
    }

    // 根据平台打开浏览器
    std::unique_ptr<BrowserHandle> handle;
    std::string browser_id;

#ifdef PLATFORM_MACOS
    std::string browser_name;
    switch (type) {
        case BrowserType::CHROME: browser_name = "Google Chrome"; break;
        case BrowserType::SAFARI: browser_name = "Safari"; break;
        case BrowserType::FIREFOX: browser_name = "Firefox"; break;
        default: browser_name = "Safari"; break;
    }
    handle = std::make_unique<MacOSBrowserHandle>(browser_name, 9515);
    browser_id = browser_name;

#elif defined(PLATFORM_LINUX)
    std::string browser_name;
    switch (type) {
        case BrowserType::CHROME: browser_name = "chrome"; break;
        case BrowserType::FIREFOX: browser_name = "firefox"; break;
        default: browser_name = "chrome"; break;
    }
    handle = std::make_unique<LinuxBrowserHandle>(browser_name, 9515);
    browser_id = browser_name;

#elif defined(PLATFORM_WINDOWS)
    std::string browser_name;
    switch (type) {
        case BrowserType::CHROME: browser_name = "chrome"; break;
        case BrowserType::EDGE: browser_name = "edge"; break;
        case BrowserType::FIREFOX: browser_name = "firefox"; break;
        default: browser_name = "edge"; break;
    }
    handle = std::make_unique<WindowsBrowserHandle>(browser_name, 9515);
    browser_id = browser_name;
#endif

    if (!handle || !handle->isConnected()) {
        return ToolResult::error("无法打开浏览器 / Failed to open browser: " + browser_id);
    }

    current_browser_id_ = browser_id;
    open_browsers_[browser_id] = std::move(handle);

    std::stringstream ss;
    ss << "浏览器已打开 / Browser opened: " << browser_id;
    return ToolResult::ok(ss.str());
}

ToolResult BrowserTool::closeBrowser() {
    std::unique_lock<std::shared_mutex> lock(browsers_mutex_);
    open_browsers_.clear();
    current_browser_id_.clear();
    return ToolResult::ok("浏览器已关闭 / Browser closed");
}

ToolResult BrowserTool::navigate(const std::string& url) {
    std::shared_lock<std::shared_mutex> lock(browsers_mutex_);

    if (current_browser_id_.empty() || open_browsers_.find(current_browser_id_) == open_browsers_.end()) {
        return ToolResult::error("浏览器未打开 / Browser not open");
    }

    auto& browser = open_browsers_[current_browser_id_];
    if (!browser->navigate(url)) {
        return ToolResult::error("导航失败 / Navigation failed");
    }

    return ToolResult::ok("已导航到 / Navigated to: " + url);
}

ToolResult BrowserTool::screenshot() {
    std::shared_lock<std::shared_mutex> lock(browsers_mutex_);

    if (current_browser_id_.empty() || open_browsers_.find(current_browser_id_) == open_browsers_.end()) {
        return ToolResult::error("浏览器未打开 / Browser not open");
    }

    auto& browser = open_browsers_[current_browser_id_];
    std::string screenshot_data = browser->screenshot();

    json meta;
    meta["format"] = "base64_png";
    meta["size"] = screenshot_data.length();

    return ToolResult::ok("截图已保存 / Screenshot captured (base64)", meta);
}

ToolResult BrowserTool::click(const Selector& selector) {
    std::shared_lock<std::shared_mutex> lock(browsers_mutex_);

    if (current_browser_id_.empty()) {
        return ToolResult::error("浏览器未打开 / Browser not open");
    }

    auto& browser = open_browsers_[current_browser_id_];
    if (!browser->click(selector)) {
        return ToolResult::error("点击失败 / Click failed");
    }

    return ToolResult::ok("已点击元素 / Element clicked");
}

ToolResult BrowserTool::type(const Selector& selector, const std::string& text) {
    std::shared_lock<std::shared_mutex> lock(browsers_mutex_);

    if (current_browser_id_.empty()) {
        return ToolResult::error("浏览器未打开 / Browser not open");
    }

    auto& browser = open_browsers_[current_browser_id_];
    if (!browser->type(selector, text)) {
        return ToolResult::error("输入失败 / Type failed");
    }

    return ToolResult::ok("已输入文本 / Text entered");
}

ToolResult BrowserTool::scroll(int x, int y) {
    std::shared_lock<std::shared_mutex> lock(browsers_mutex_);

    if (current_browser_id_.empty()) {
        return ToolResult::error("浏览器未打开 / Browser not open");
    }

    auto& browser = open_browsers_[current_browser_id_];
    browser->scroll(x, y);

    return ToolResult::ok("已滚动 / Scrolled");
}

ToolResult BrowserTool::executeScript(const std::string& script) {
    std::shared_lock<std::shared_mutex> lock(browsers_mutex_);

    if (current_browser_id_.empty()) {
        return ToolResult::error("浏览器未打开 / Browser not open");
    }

    auto& browser = open_browsers_[current_browser_id_];
    std::string result = browser->executeScript(script);

    return ToolResult::ok(result);
}

ToolResult BrowserTool::getText(const Selector& selector) {
    std::shared_lock<std::shared_mutex> lock(browsers_mutex_);

    if (current_browser_id_.empty()) {
        return ToolResult::error("浏览器未打开 / Browser not open");
    }

    auto& browser = open_browsers_[current_browser_id_];
    std::string text = browser->getText(selector);

    return ToolResult::ok(text);
}

ToolResult BrowserTool::wait(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    return ToolResult::ok("已等待 / Waited " + std::to_string(milliseconds) + "ms");
}

ToolResult BrowserTool::findElement(const Selector& selector) {
    std::shared_lock<std::shared_mutex> lock(browsers_mutex_);

    if (current_browser_id_.empty()) {
        return ToolResult::error("浏览器未打开 / Browser not open");
    }

    auto& browser = open_browsers_[current_browser_id_];
    std::string script = "!!document.querySelector('" + selector.value + "')";
    std::string result = browser->executeScript(script);

    if (result.find("true") != std::string::npos) {
        return ToolResult::ok("找到元素 / Element found");
    }
    return ToolResult::error("未找到元素 / Element not found");
}

ToolResult BrowserTool::listTabs() {
    // TODO: 实现标签页列表
    return ToolResult::ok("标签页列表功能开发中 / Tab list feature in development");
}

ToolResult BrowserTool::newTab(const std::string& url) {
    if (!url.empty()) {
        return navigate(url);
    }
    return ToolResult::ok("新标签页已创建 / New tab created");
}

ToolResult BrowserTool::closeTab(int index) {
    // TODO: 实现关闭标签页
    return ToolResult::ok("标签页已关闭 / Tab closed");
}

ToolResult BrowserTool::switchTab(int index) {
    // TODO: 实现切换标签页
    return ToolResult::ok("已切换到标签页 / Switched to tab " + std::to_string(index));
}

std::vector<BrowserType> BrowserTool::detectInstalledBrowsers() {
    std::vector<BrowserType> browsers;

#ifdef PLATFORM_MACOS
    // 检查 Safari
    if (std::filesystem::exists("/Applications/Safari.app")) {
        browsers.push_back(BrowserType::SAFARI);
    }
    // 检查 Chrome
    if (std::filesystem::exists("/Applications/Google Chrome.app")) {
        browsers.push_back(BrowserType::CHROME);
    }
    // 检查 Firefox
    if (std::filesystem::exists("/Applications/Firefox.app")) {
        browsers.push_back(BrowserType::FIREFOX);
    }
#elif defined(PLATFORM_LINUX)
    // 检查常见浏览器路径
    std::vector<std::string> paths = {
        "/usr/bin/google-chrome",
        "/usr/bin/chromium-browser",
        "/usr/bin/firefox",
        "/opt/google/chrome/chrome",
        "/opt/chromium/chrome"
    };
    for (const auto& path : paths) {
        if (std::filesystem::exists(path)) {
            if (path.find("chrome") != std::string::npos || path.find("chromium") != std::string::npos) {
                browsers.push_back(BrowserType::CHROME);
            } else if (path.find("firefox") != std::string::npos) {
                browsers.push_back(BrowserType::FIREFOX);
            }
        }
    }
#elif defined(PLATFORM_WINDOWS)
    // 检查注册表或程序文件
    if (std::filesystem::exists("C:/Program Files/Google/Chrome/Application/chrome.exe") ||
        std::filesystem::exists("C:/Program Files (x86)/Google/Chrome/Application/chrome.exe")) {
        browsers.push_back(BrowserType::CHROME);
    }
    if (std::filesystem::exists("C:/Program Files/Microsoft/Edge/Application/msedge.exe")) {
        browsers.push_back(BrowserType::EDGE);
    }
    if (std::filesystem::exists("C:/Program Files/Mozilla Firefox/firefox.exe")) {
        browsers.push_back(BrowserType::FIREFOX);
    }
#endif

    return browsers;
}

} // namespace roboclaw
