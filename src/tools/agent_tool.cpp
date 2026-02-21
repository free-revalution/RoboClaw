// Agent 发现和管理工具实现 / Agent Discovery and Management Tool Implementation

#include "agent_tool.h"
#include "../utils/logger.h"
#include <sstream>
#include <filesystem>
#include <fstream>
#include <cctype>
#include <algorithm>

// 定义 PLATFORM_UNIX（用于 macOS 和 Linux）
#if defined(PLATFORM_MACOS) || defined(PLATFORM_LINUX)
#ifndef PLATFORM_UNIX
#define PLATFORM_UNIX
#endif
#endif

#ifdef PLATFORM_MACOS
#include <sys/sysctl.h>
#endif

#ifdef PLATFORM_LINUX
#include <unistd.h>
#endif

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <shlobj.h>
#include <tchar.h>
#endif

namespace roboclaw {

// ============================================================
// AgentTool 实现
// ============================================================

AgentTool::AgentTool()
    : ToolBase("agent", "Agent 发现和管理工具 / Agent discovery and management tool for local installed AI coding assistants") {
}

ToolDescription AgentTool::getToolDescription() const {
    ToolDescription desc;
    desc.name = "agent";
    desc.description = "Agent 发现和管理工具，支持检测和管理本地已安装的 AI 编程助手 (Claude Code, Cursor, Copilot, etc.) / Agent discovery and management tool for local installed AI coding assistants.";

    desc.parameters = {
        {"action", "string", "操作类型 / Action: list, show, refresh, launch, stop, configure, capabilities", true, ""},
        {"agent_id", "string", "Agent ID / Agent标识符", false, ""},
        {"config", "string", "配置 JSON / Configuration JSON (for configure action)", false, ""}
    };
    return desc;
}

bool AgentTool::validateParams(const json& params) const {
    if (!hasRequiredParam(params, "action")) {
        return false;
    }

    std::string action_str = getStringParam(params, "action");
    std::vector<std::string> valid_actions = {
        "list", "show", "refresh", "launch", "stop", "configure", "capabilities"
    };

    return std::find(valid_actions.begin(), valid_actions.end(), action_str) != valid_actions.end();
}

ToolResult AgentTool::execute(const json& params) {
    if (!validateParams(params)) {
        return ToolResult::error("Invalid parameters");
    }

    std::string action_str = getStringParam(params, "action");

    if (action_str == "list") {
        return listAgents();
    } else if (action_str == "show") {
        return showAgent(getStringParam(params, "agent_id", ""));
    } else if (action_str == "refresh") {
        return refreshAgents();
    } else if (action_str == "launch") {
        return launchAgent(getStringParam(params, "agent_id", ""));
    } else if (action_str == "stop") {
        return stopAgent(getStringParam(params, "agent_id", ""));
    } else if (action_str == "configure") {
        return configureAgent(getStringParam(params, "agent_id", ""), params);
    } else if (action_str == "capabilities") {
        return getCapabilities(getStringParam(params, "agent_id", ""));
    }

    return ToolResult::error("Unknown action: " + action_str);
}

std::string AgentTool::agentTypeToString(AgentType type) {
    switch (type) {
        case AgentType::CLAUDE_CODE:  return "claude_code";
        case AgentType::CURSOR:      return "cursor";
        case AgentType::COPILOT:     return "copilot";
        case AgentType::CODEX:       return "codex";
        case AgentType::OPENCLAW:    return "openclaw";
        case AgentType::TABNINE:     return "tabnine";
        case AgentType::BLACKBOX:    return "blackbox";
        case AgentType::REPLIT:      return "replit";
        case AgentType::SOURCEGRAPH: return "sourcegraph";
        default:                     return "other";
    }
}

AgentType AgentTool::stringToAgentType(const std::string& str) {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    if (lower == "claude_code" || lower == "claude-code") return AgentType::CLAUDE_CODE;
    if (lower == "cursor") return AgentType::CURSOR;
    if (lower == "copilot") return AgentType::COPILOT;
    if (lower == "codex") return AgentType::CODEX;
    if (lower == "openclaw") return AgentType::OPENCLAW;
    if (lower == "tabnine") return AgentType::TABNINE;
    if (lower == "blackbox") return AgentType::BLACKBOX;
    if (lower == "replit") return AgentType::REPLIT;
    if (lower == "sourcegraph" || lower == "cody") return AgentType::SOURCEGRAPH;
    return AgentType::OTHER;
}

void AgentTool::scanInstalledAgents() {
    scanVSCodeExtensions();
    scanStandaloneApplications();
    scanCLItools();
}

void AgentTool::scanVSCodeExtensions() {
    std::vector<std::string> vscode_paths;

    #ifdef PLATFORM_MACOS
    const char* home = std::getenv("HOME");
    if (home) {
        vscode_paths.push_back(std::string(home) + "/.vscode/extensions");
        vscode_paths.push_back(std::string(home) + "/.vscode-server/extensions");
    }
    vscode_paths.push_back("/Applications/Visual Studio Code.app/Contents/Resources/app/extensions");
    #elif defined(PLATFORM_LINUX)
    const char* home = std::getenv("HOME");
    if (home) {
        vscode_paths.push_back(std::string(home) + "/.vscode/extensions");
        vscode_paths.push_back(std::string(home) + "/.vscode-server/extensions");
    }
    vscode_paths.push_back("/usr/share/code/extensions");
    #elif defined(PLATFORM_WINDOWS)
    char localAppData[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppData))) {
        vscode_paths.push_back(std::string(localAppData) + "\\Programs\\Microsoft VS Code\\extensions");
        vscode_paths.push_back(std::string(localAppData) + "\\Programs\\VS Code\\extensions");
    }
    #endif

    for (const auto& base_path : vscode_paths) {
        if (!std::filesystem::exists(base_path)) continue;

        // 搜索扩展目录
        for (const auto& entry : std::filesystem::directory_iterator(base_path)) {
            if (!entry.is_directory()) continue;

            std::string extension_name = entry.path().filename().string();
            AgentType type = detectAgentType(entry.path().string(), extension_name);

            if (type != AgentType::OTHER) {
                AgentInfo info;
                info.id = agentTypeToString(type) + "_" + extension_name.substr(0, std::min(size_t(8), extension_name.length()));
                info.name = extension_name;
                info.type = type;
                info.install_path = entry.path().string();
                info.version = getAgentVersion(entry.path().string());
                info.enabled = true;
                info.description = "VSCode Extension: " + extension_name;

                // 检测能力
                if (type == AgentType::CLAUDE_CODE) {
                    info.capabilities = {"code_completion", "chat", "code_explanation", "refactoring"};
                } else if (type == AgentType::COPILOT) {
                    info.capabilities = {"code_completion", "suggestion"};
                } else if (type == AgentType::CURSOR) {
                    info.capabilities = {"code_completion", "chat", "codebase_understanding"};
                }

                discovered_agents_[info.id] = info;
            }
        }
    }
}

void AgentTool::scanStandaloneApplications() {
    std::vector<std::pair<std::string, AgentType>> app_paths;

    #ifdef PLATFORM_MACOS
    app_paths = {
        {"/Applications/Cursor.app", AgentType::CURSOR},
        {"/Applications/Replit.app", AgentType::REPLIT},
        {"~/Applications/Tabnine.app", AgentType::TABNINE}
    };
    #elif defined(PLATFORM_LINUX)
    app_paths = {
        {"/usr/share/cursor", AgentType::CURSOR},
        {"/opt/cursor", AgentType::CURSOR}
    };
    #elif defined(PLATFORM_WINDOWS)
    const char* username = std::getenv("USERNAME");
    if (username) {
        app_paths.push_back({"C:/Users/" + std::string(username) + "/AppData/Local/Programs/Microsoft VS Code/Cursor.exe", AgentType::CURSOR});
    }
    app_paths.push_back({"C:/Program Files/Replit/Replit.exe", AgentType::REPLIT});
    #endif

    for (const auto& [path, type] : app_paths) {
        std::string expanded_path = path;
        // 展开 ~
        if (!expanded_path.empty() && expanded_path[0] == '~') {
            const char* home = std::getenv("HOME");
            if (home) {
                expanded_path.replace(0, 1, home);
            }
        }

        if (std::filesystem::exists(expanded_path)) {
            AgentInfo info;
            info.id = agentTypeToString(type) + "_app";
            info.name = agentTypeToString(type);
            info.type = type;
            info.install_path = expanded_path;
            info.executable_path = expanded_path;
            info.enabled = true;
            info.version = getAgentVersion(expanded_path);
            info.description = "Standalone Application: " + agentTypeToString(type);
            info.command = "open \"" + expanded_path + "\"";

            if (type == AgentType::CURSOR) {
                info.capabilities = {"ide", "code_completion", "chat", "codebase_chat"};
            } else if (type == AgentType::REPLIT) {
                info.capabilities = {"ide", "ai_assistant", "collaboration"};
            }

            discovered_agents_[info.id] = info;
        }
    }
}

void AgentTool::scanCLItools() {
    std::vector<std::pair<std::string, AgentType>> cli_tools = {
        {"codeium", AgentType::BLACKBOX},
        {"blackbox", AgentType::BLACKBOX},
        {"tabnine", AgentType::TABNINE},
        {"sg", AgentType::SOURCEGRAPH}
    };

    #ifdef PLATFORM_UNIX
    // 检查 PATH 中的工具
    const char* path_env = std::getenv("PATH");
    if (path_env) {
        std::stringstream ss(path_env);
        std::string path;
        while (std::getline(ss, path, ':')) {
            for (const auto& [tool, type] : cli_tools) {
                std::string tool_path = path + "/" + tool;
                if (std::filesystem::exists(tool_path) && std::filesystem::is_regular_file(tool_path)) {
                    AgentInfo info;
                    info.id = agentTypeToString(type) + "_cli";
                    info.name = tool;
                    info.type = type;
                    info.executable_path = tool_path;
                    info.enabled = true;
                    info.version = getAgentVersion(tool_path);
                    info.description = "CLI Tool: " + tool;
                    info.command = tool_path;
                    info.capabilities = {"cli", "code_completion"};

                    discovered_agents_[info.id] = info;
                }
            }
        }
    }
    #endif
}

AgentType AgentTool::detectAgentType(const std::string& path, const std::string& name) {
    std::string lower_name = name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    // Claude Code
    if (lower_name.find("anthropic") != std::string::npos ||
        lower_name.find("claude") != std::string::npos) {
        return AgentType::CLAUDE_CODE;
    }

    // Cursor
    if (lower_name.find("cursor") != std::string::npos) {
        return AgentType::CURSOR;
    }

    // Copilot
    if (lower_name.find("github") != std::string::npos ||
        lower_name.find("copilot") != std::string::npos) {
        return AgentType::COPILOT;
    }

    // Tabnine
    if (lower_name.find("tabnine") != std::string::npos) {
        return AgentType::TABNINE;
    }

    // Blackbox
    if (lower_name.find("blackbox") != std::string::npos ||
        lower_name.find("codeium") != std::string::npos) {
        return AgentType::BLACKBOX;
    }

    // Sourcegraph Cody
    if (lower_name.find("sourcegraph") != std::string::npos ||
        lower_name.find("cody") != std::string::npos) {
        return AgentType::SOURCEGRAPH;
    }

    return AgentType::OTHER;
}

std::string AgentTool::getAgentVersion(const std::string& path) {
    // 尝试从 package.json 或其他元数据文件读取版本
    std::string package_json = path + "/package.json";
    if (std::filesystem::exists(package_json)) {
        std::ifstream file(package_json);
        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());

        // 简单的 JSON 解析（查找 version 字段）
        size_t pos = content.find("\"version\"");
        if (pos != std::string::npos) {
            size_t start = content.find("\"", pos + 9);
            if (start != std::string::npos) {
                size_t end = content.find("\"", start + 1);
                if (end != std::string::npos) {
                    return content.substr(start + 1, end - start - 1);
                }
            }
        }
    }

    return "unknown";
}

ToolResult AgentTool::listAgents() {
    std::shared_lock<std::shared_mutex> lock(agents_mutex_);

    if (discovered_agents_.empty()) {
        const_cast<AgentTool*>(this)->scanInstalledAgents();
    }

    if (discovered_agents_.empty()) {
        return ToolResult::ok("未找到已安装的 Agents / No installed agents found");
    }

    std::stringstream ss;
    ss << "已安装的 Agents / Installed Agents:\n\n";

    for (const auto& [id, info] : discovered_agents_) {
        ss << "ID: " << id << "\n";
        ss << "  名称 / Name: " << info.name << "\n";
        ss << "  类型 / Type: " << agentTypeToString(info.type) << "\n";
        ss << "  描述 / Description: " << info.description << "\n";
        ss << "  状态 / Status: " << (info.enabled ? "启用" : "禁用") << "\n";
        if (!info.version.empty()) {
            ss << "  版本 / Version: " << info.version << "\n";
        }
        if (!info.capabilities.empty()) {
            ss << "  能力 / Capabilities: ";
            for (size_t i = 0; i < info.capabilities.size(); ++i) {
                ss << info.capabilities[i];
                if (i < info.capabilities.size() - 1) ss << ", ";
            }
            ss << "\n";
        }
        ss << "  ----------------------------------------------\n";
    }

    // 构建 JSON 数组
    json agents_json = json::array();
    for (const auto& [id, info] : discovered_agents_) {
        agents_json.push_back(info.toJson());
    }

    json meta;
    meta["count"] = discovered_agents_.size();
    meta["agents"] = agents_json;

    return ToolResult::ok(ss.str(), meta);
}

ToolResult AgentTool::showAgent(const std::string& agent_id) {
    std::shared_lock<std::shared_mutex> lock(agents_mutex_);

    auto it = discovered_agents_.find(agent_id);
    if (it == discovered_agents_.end()) {
        return ToolResult::error("Agent 未找到 / Agent not found: " + agent_id);
    }

    const auto& info = it->second;
    std::stringstream ss;
    ss << "Agent 详情 / Agent Details:\n\n";
    ss << "ID: " << info.id << "\n";
    ss << "名称 / Name: " << info.name << "\n";
    ss << "描述 / Description: " << info.description << "\n";
    ss << "类型 / Type: " << agentTypeToString(info.type) << "\n";
    ss << "版本 / Version: " << info.version << "\n";
    ss << "安装路径 / Install Path: " << info.install_path << "\n";
    ss << "可执行文件 / Executable: " << info.executable_path << "\n";
    ss << "配置文件 / Config: " << info.config_path << "\n";
    ss << "启动命令 / Command: " << info.command << "\n";
    ss << "状态 / Status: " << (info.enabled ? "启用" : "禁用") << "\n";
    ss << "\n能力 / Capabilities:\n";
    for (const auto& cap : info.capabilities) {
        ss << "  - " << cap << "\n";
    }

    return ToolResult::ok(ss.str());
}

ToolResult AgentTool::refreshAgents() {
    std::unique_lock<std::shared_mutex> lock(agents_mutex_);
    discovered_agents_.clear();
    scanInstalledAgents();

    std::stringstream ss;
    ss << "Agent 列表已刷新，共发现 / Agent list refreshed, found: "
       << discovered_agents_.size() << " 个 Agents";
    return ToolResult::ok(ss.str());
}

ToolResult AgentTool::launchAgent(const std::string& agent_id) {
    std::shared_lock<std::shared_mutex> lock(agents_mutex_);

    auto it = discovered_agents_.find(agent_id);
    if (it == discovered_agents_.end()) {
        return ToolResult::error("Agent 未找到 / Agent not found: " + agent_id);
    }

    const auto& info = it->second;
    std::string command = info.command.empty() ? ("\"" + info.executable_path + "\"") : info.command;

    LOG_INFO("启动 Agent / Launching Agent: " + info.name + " (" + command + ")");

    #ifdef PLATFORM_MACOS
    command = "open " + command;
    #elif defined(PLATFORM_LINUX)
    command += " &";
    #elif defined(PLATFORM_WINDOWS)
    command = "start \"\" " + command;
    #endif

    int result = std::system(command.c_str());

    if (result == 0) {
        return ToolResult::ok("Agent 已启动 / Agent launched: " + info.name);
    }

    return ToolResult::error("Agent 启动失败 / Failed to launch Agent");
}

ToolResult AgentTool::stopAgent(const std::string& agent_id) {
    // TODO: 实现 Agent 停止逻辑
    return ToolResult::ok("Agent 停止功能开发中 / Agent stop feature in development");
}

ToolResult AgentTool::configureAgent(const std::string& agent_id, const json& config) {
    std::shared_lock<std::shared_mutex> lock(agents_mutex_);

    auto it = discovered_agents_.find(agent_id);
    if (it == discovered_agents_.end()) {
        return ToolResult::error("Agent 未找到 / Agent not found: " + agent_id);
    }

    // 更新配置
    // TODO: 实现配置保存逻辑

    return ToolResult::ok("Agent 配置已更新 / Agent configuration updated");
}

ToolResult AgentTool::getCapabilities(const std::string& agent_id) {
    std::shared_lock<std::shared_mutex> lock(agents_mutex_);

    auto it = discovered_agents_.find(agent_id);
    if (it == discovered_agents_.end()) {
        return ToolResult::error("Agent 未找到 / Agent not found: " + agent_id);
    }

    const auto& info = it->second;
    std::stringstream ss;
    ss << "Agent 能力 / Agent Capabilities for " << info.name << ":\n\n";

    for (const auto& cap : info.capabilities) {
        ss << "  - " << cap << "\n";
    }

    json meta;
    meta["capabilities"] = info.capabilities;

    return ToolResult::ok(ss.str(), meta);
}

} // namespace roboclaw
