// 交互式对话模式 - InteractiveMode
// 处理用户与Agent的交互式对话

#ifndef ROBOCLAW_CLI_INTERACTIVE_MODE_H
#define ROBOCLAW_CLI_INTERACTIVE_MODE_H

#include "../agent/agent.h"
#include "../session/session_manager.h"
#include "../storage/config_manager.h"
#include <string>
#include <functional>

namespace roboclaw {

// 交互模式配置
struct InteractiveConfig {
    bool show_thinking;      // 显示思考过程
    bool show_tool_calls;    // 显示工具调用详情
    bool stream_output;      // 流式输出
    int max_history;         // 最大历史记录数

    InteractiveConfig()
        : show_thinking(true)
        , show_tool_calls(true)
        , stream_output(true)
        , max_history(100) {}
};

// 交互模式
class InteractiveMode {
public:
    InteractiveMode(std::shared_ptr<Agent> agent,
                    std::shared_ptr<SessionManager> sessionMgr,
                    const ConfigManager& configMgr);

    ~InteractiveMode() = default;

    // 设置配置
    void setConfig(const InteractiveConfig& config) { config_ = config; }

    // 运行交互模式
    void run();

    // 处理单条消息
    bool processMessage(const std::string& message);

    // 退出标志
    void setExitFlag(bool exit) { should_exit_ = exit; }
    bool shouldExit() const { return should_exit_; }

private:
    // 显示欢迎信息
    void showWelcome();

    // 显示提示符
    void showPrompt();

    // 读取用户输入
    std::string readInput();

    // 处理命令
    bool handleCommand(const std::string& input);

    // 显示帮助
    void showHelp();

    // 显示会话信息
    void showSessionInfo();

    // 切换分支
    void switchBranch(const std::string& branchName);

    // 创建分支
    void createBranch(const std::string& branchName);

    // 列出分支
    void listBranches();

    // 显示Agent响应
    void displayResponse(const AgentResponse& response);

    // 显示工具调用
    void displayToolCall(const ChatMessage::ToolCall& call, const ToolResult& result);

    // 显示横幅
    void showBanner();

    // 处理斜杠命令
    bool handleSlashCommand(const std::string& command);

    // 斜杠命令处理函数
    bool cmdHelp();
    bool cmdConfig();
    bool cmdClear();
    bool cmdAgent(const std::string& args);
    bool cmdBrowser(const std::string& args);

    // 保存当前会话
    void saveCurrentSession();

    // 成员变量
    std::shared_ptr<Agent> agent_;
    std::shared_ptr<SessionManager> session_manager_;
    const ConfigManager& config_manager_;
    InteractiveConfig config_;

    bool should_exit_;
    std::string current_input_;
};

} // namespace roboclaw

#endif // ROBOCLAW_CLI_INTERACTIVE_MODE_H
