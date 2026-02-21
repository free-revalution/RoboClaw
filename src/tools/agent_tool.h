// Agent 发现和管理工具 / Agent Discovery and Management Tool
// 用于读取和管理本地已安装的 Agents (Claude Code, Cursor, Copilot, etc.)

#ifndef ROBOCLAW_TOOLS_AGENT_TOOL_H
#define ROBOCLAW_TOOLS_AGENT_TOOL_H

#include "tool_base.h"
#include <string>
#include <vector>
#include <map>

namespace roboclaw {

// Agent 类型
enum class AgentType {
    CLAUDE_CODE,     // Claude Code (VSCode extension)
    CURSOR,          // Cursor AI IDE
    COPILOT,         // GitHub Copilot
    CODEX,           // OpenAI Codex
    OPENCLAW,        // OpenClaw
    TABNINE,         // Tabnine
    BLACKBOX,        // Blackbox AI
    REPLIT,          // Replit Ghostwriter
    SOURCEGRAPH,     // Sourcegraph Cody
    OTHER            // 其他
};

// Agent 信息
struct AgentInfo {
    std::string id;              // 唯一标识
    std::string name;            // 名称
    std::string description;     // 描述
    AgentType type;              // 类型
    std::string version;         // 版本
    std::string install_path;    // 安装路径
    std::string executable_path; // 可执行文件路径
    std::string config_path;     // 配置文件路径
    bool enabled;                // 是否启用
    std::string command;         // 启动命令
    std::vector<std::string> capabilities;  // 能力列表

    json toJson() const {
        json j;
        j["id"] = id;
        j["name"] = name;
        j["description"] = description;
        j["type"] = static_cast<int>(type);
        j["version"] = version;
        j["install_path"] = install_path;
        j["executable_path"] = executable_path;
        j["config_path"] = config_path;
        j["enabled"] = enabled;
        j["command"] = command;
        j["capabilities"] = capabilities;
        return j;
    }
};

// Agent 工具类
class AgentTool : public ToolBase {
public:
    AgentTool();
    ~AgentTool() override = default;

    // 获取工具描述
    ToolDescription getToolDescription() const override;

    // 验证参数
    bool validateParams(const json& params) const override;

    // 执行工具
    ToolResult execute(const json& params) override;

    // 列出所有已安装的 Agents
    ToolResult listAgents();

    // 显示 Agent 详情
    ToolResult showAgent(const std::string& agent_id);

    // 刷新 Agent 列表（重新扫描）
    ToolResult refreshAgents();

    // 启动指定 Agent
    ToolResult launchAgent(const std::string& agent_id);

    // 停止 Agent
    ToolResult stopAgent(const std::string& agent_id);

    // 配置 Agent
    ToolResult configureAgent(const std::string& agent_id, const json& config);

    // 获取 Agent 能力
    ToolResult getCapabilities(const std::string& agent_id);

private:
    // 扫描已安装的 Agents
    void scanInstalledAgents();

    // 扫描 VSCode 扩展
    void scanVSCodeExtensions();

    // 扫描独立应用
    void scanStandaloneApplications();

    // 扫描 CLI 工具
    void scanCLItools();

    // 检测 Agent 类型
    AgentType detectAgentType(const std::string& path, const std::string& name);

    // 获取 Agent 版本
    std::string getAgentVersion(const std::string& path);

    // 类型转换辅助函数
    static std::string agentTypeToString(AgentType type);
    static AgentType stringToAgentType(const std::string& str);

    // 已发现的 Agents
    std::map<std::string, AgentInfo> discovered_agents_;

    // 读写锁保证线程安全
    mutable std::shared_mutex agents_mutex_;
};

} // namespace roboclaw

#endif // ROBOCLAW_TOOLS_AGENT_TOOL_H
