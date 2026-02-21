// 浏览器自动化工具 - Browser Automation Tool
// 类似 OpenClaw 的可视化浏览器操作功能 / Visual browser control like OpenClaw

#ifndef ROBOCLAW_TOOLS_BROWSER_TOOL_H
#define ROBOCLAW_TOOLS_BROWSER_TOOL_H

#include "tool_base.h"
#include <string>
#include <memory>
#include <map>

namespace roboclaw {

// 浏览器类型
enum class BrowserType {
    CHROME,
    FIREFOX,
    SAFARI,
    EDGE,
    AUTO  // 自动检测
};

// 浏览器操作类型
enum class BrowserAction {
    OPEN,           // 打开浏览器
    CLOSE,          // 关闭浏览器
    NAVIGATE,       // 导航到 URL
    SCREENSHOT,     // 截图
    CLICK,          // 点击元素
    TYPE,           // 输入文本
    SCROLL,         // 滚动
    WAIT,           // 等待
    EXECUTE,        // 执行 JavaScript
    GET_TEXT,       // 获取文本
    GET_HTML,       // 获取 HTML
    FIND_ELEMENT,   // 查找元素
    LIST_TABS,      // 列出标签页
    NEW_TAB,        // 新建标签页
    CLOSE_TAB,      // 关闭标签页
    SWITCH_TAB      // 切换标签页
};

// 元素定位器
struct Selector {
    std::string type;      // css, xpath, id, name, class
    std::string value;     // 定位器值

    json toJson() const {
        json j;
        j["type"] = type;
        j["value"] = value;
        return j;
    }
};

// 浏览器句柄（跨平台）
class BrowserHandle {
public:
    virtual ~BrowserHandle() = default;
    virtual bool isConnected() const = 0;
    virtual bool navigate(const std::string& url) = 0;
    virtual bool click(const Selector& selector) = 0;
    virtual bool type(const Selector& selector, const std::string& text) = 0;
    virtual std::string screenshot() = 0;
    virtual std::string getText(const Selector& selector) = 0;
    virtual std::string executeScript(const std::string& script) = 0;
    virtual bool scroll(int x, int y) = 0;
    virtual void close() = 0;

    std::string getBrowserName() const { return browser_name_; }
    int getPort() const { return port_; }

protected:
    // Protected constructor for derived classes
    BrowserHandle(const std::string& browser_name, int port)
        : browser_name_(browser_name), port_(port) {}

    std::string browser_name_;
    int port_;  // WebDriver 端口
};

// 浏览器工具类
class BrowserTool : public ToolBase {
public:
    BrowserTool();
    ~BrowserTool() override;

    // 获取工具描述
    ToolDescription getToolDescription() const override;

    // 验证参数
    bool validateParams(const json& params) const override;

    // 执行工具
    ToolResult execute(const json& params) override;

    // 打开浏览器
    ToolResult openBrowser(BrowserType type = BrowserType::AUTO);

    // 关闭浏览器
    ToolResult closeBrowser();

    // 导航到 URL
    ToolResult navigate(const std::string& url);

    // 截图
    ToolResult screenshot();

    // 点击元素
    ToolResult click(const Selector& selector);

    // 输入文本
    ToolResult type(const Selector& selector, const std::string& text);

    // 滚动
    ToolResult scroll(int x, int y);

    // 执行 JavaScript
    ToolResult executeScript(const std::string& script);

    // 获取文本
    ToolResult getText(const Selector& selector);

    // 等待
    ToolResult wait(int milliseconds);

    // 查找元素
    ToolResult findElement(const Selector& selector);

    // 列出标签页
    ToolResult listTabs();

    // 新建标签页
    ToolResult newTab(const std::string& url = "");

    // 关闭标签页
    ToolResult closeTab(int index = -1);

    // 切换标签页
    ToolResult switchTab(int index);

private:
    // 解析定位器
    Selector parseSelector(const json& params) const;

    // 打开浏览器（平台相关）
    #ifdef PLATFORM_MACOS
    std::unique_ptr<BrowserHandle> openBrowserMacOS(BrowserType type);
    #endif
    #ifdef PLATFORM_LINUX
    std::unique_ptr<BrowserHandle> openBrowserLinux(BrowserType type);
    #endif
    #ifdef PLATFORM_WINDOWS
    std::unique_ptr<BrowserHandle> openBrowserWindows(BrowserType type);
    #endif

    // 检测已安装的浏览器
    std::vector<BrowserType> detectInstalledBrowsers();

    // WebDriver 管理
    std::map<std::string, std::unique_ptr<BrowserHandle>> open_browsers_;
    std::string current_browser_id_;

    // 读写锁保证线程安全
    mutable std::shared_mutex browsers_mutex_;
};

} // namespace roboclaw

#endif // ROBOCLAW_TOOLS_BROWSER_TOOL_H
