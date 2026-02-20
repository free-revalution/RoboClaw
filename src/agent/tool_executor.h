// 工具执行器 - ToolExecutor
// 统一管理所有工具的执行

#ifndef ROBOCLAW_AGENT_TOOL_EXECUTOR_H
#define ROBOCLAW_AGENT_TOOL_EXECUTOR_H

#include "tool_base.h"
#include "../storage/config_manager.h"
#include <memory>
#include <map>

namespace roboclaw {

// 工具执行请求
struct ToolExecutionRequest {
    std::string tool_name;
    json parameters;

    json toJson() const {
        json j;
        j["tool"] = tool_name;
        j["parameters"] = parameters;
        return j;
    }

    static ToolExecutionRequest fromJson(const json& j) {
        ToolExecutionRequest req;
        if (j.contains("tool")) {
            req.tool_name = j["tool"].get<std::string>();
        }
        if (j.contains("parameters")) {
            req.parameters = j["parameters"];
        }
        return req;
    }
};

// 工具执行器
class ToolExecutor {
public:
    ToolExecutor();
    ~ToolExecutor() = default;

    // 初始化工具
    void initialize();

    // 注册工具
    void registerTool(const std::string& name, std::shared_ptr<ToolBase> tool);

    // 执行工具
    ToolResult execute(const ToolExecutionRequest& request);

    // 执行工具（通过JSON）
    ToolResult executeJson(const json& requestJson);

    // 执行工具（通过名称和参数）
    ToolResult execute(const std::string& toolName, const json& parameters);

    // 获取工具描述
    std::vector<ToolDescription> getAllToolDescriptions() const;

    // 获取工具Schema（用于发送给LLM）
    json getToolsSchema() const;

    // 检查工具是否存在
    bool hasTool(const std::string& name) const;

    // 获取工具
    std::shared_ptr<ToolBase> getTool(const std::string& name);

private:
    ToolRegistry& registry_;

    // 从配置加载工具设置
    void loadToolSettings(const ConfigManager& config_mgr);
};

} // namespace roboclaw

#endif // ROBOCLAW_AGENT_TOOL_EXECUTOR_H
