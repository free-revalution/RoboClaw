// Bash工具 - 执行shell命令

#ifndef ROBOCLAW_TOOLS_BASH_TOOL_H
#define ROBOCLAW_TOOLS_BASH_TOOL_H

#include "tool_base.h"
#include <string>
#include <vector>

namespace roboclaw {

class BashTool : public ToolBase {
public:
    BashTool();
    ~BashTool() override = default;

    // 获取工具描述
    ToolDescription getDescription() const override;

    // 验证参数
    bool validateParams(const json& params) const override;

    // 执行工具
    ToolResult execute(const json& params) override;

    // 设置超时时间
    void setTimeout(int timeout) { default_timeout_ = timeout; }

    // 设置禁止的命令列表
    void setForbiddenCommands(const std::vector<std::string>& commands) {
        forbidden_commands_ = commands;
    }

private:
    // 执行命令
    bool executeCommand(const std::string& command, int timeout,
                       std::string& stdout, std::string& stderr,
                       int& exitCode, std::string& error);

    // 检查命令是否被禁止
    bool isCommandForbidden(const std::string& command) const;

    // 执行命令（Unix）
    #ifdef PLATFORM_UNIX
    bool executeCommandUnix(const std::string& command, int timeout,
                           std::string& stdout, std::string& stderr,
                           int& exitCode, std::string& error);
    #endif

    // 执行命令（Windows）
    #ifdef PLATFORM_WINDOWS
    bool executeCommandWindows(const std::string& command, int timeout,
                              std::string& stdout, std::string& stderr,
                              int& exitCode, std::string& error);
    #endif

    int default_timeout_;
    std::vector<std::string> forbidden_commands_;
};

// 平台检测宏
#if defined(PLATFORM_MACOS) || defined(PLATFORM_LINUX)
#define PLATFORM_UNIX
#endif

} // namespace roboclaw

#endif // ROBOCLAW_TOOLS_BASH_TOOL_H
