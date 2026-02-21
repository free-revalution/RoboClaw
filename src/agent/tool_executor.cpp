// ToolExecutor实现

#include "tool_executor.h"
#include "../tools/read_tool.h"
#include "../tools/write_tool.h"
#include "../tools/edit_tool.h"
#include "../tools/bash_tool.h"
#include "../tools/serial_tool.h"
#include "../tools/browser_tool.h"
#include "../tools/agent_tool.h"

namespace roboclaw {

ToolExecutor::ToolExecutor()
    : registry_(ToolRegistry::getInstance()) {
}

void ToolExecutor::initialize() {
    LOG_INFO("初始化工具执行器");

    // 注册基础工具
    registerTool("read", std::make_shared<ReadTool>());
    registerTool("write", std::make_shared<WriteTool>());
    registerTool("edit", std::make_shared<EditTool>());
    registerTool("bash", std::make_shared<BashTool>());
    registerTool("serial", std::make_shared<SerialTool>());
    registerTool("browser", std::make_shared<BrowserTool>());
    registerTool("agent", std::make_shared<AgentTool>());

    LOG_INFO("工具注册完成，共 " + std::to_string(registry_.getAllToolNames().size()) + " 个工具");
}

void ToolExecutor::registerTool(const std::string& name, std::shared_ptr<ToolBase> tool) {
    registry_.registerTool(name, tool);
}

ToolResult ToolExecutor::execute(const ToolExecutionRequest& request) {
    LOG_DEBUG("执行工具: " + request.tool_name);

    // 检查工具是否存在
    if (!hasTool(request.tool_name)) {
        return ToolResult::error("工具不存在: " + request.tool_name);
    }

    // 获取工具
    auto tool = getTool(request.tool_name);
    if (!tool) {
        return ToolResult::error("无法获取工具: " + request.tool_name);
    }

    // 执行工具
    try {
        ToolResult result = tool->execute(request.parameters);
        return result;
    } catch (const std::exception& e) {
        LOG_ERROR("工具执行异常: " + std::string(e.what()));
        return ToolResult::error(std::string("工具执行异常: ") + e.what());
    } catch (...) {
        LOG_ERROR("工具执行异常: 未知错误");
        return ToolResult::error("工具执行异常: 未知错误");
    }
}

ToolResult ToolExecutor::executeJson(const json& requestJson) {
    try {
        ToolExecutionRequest request = ToolExecutionRequest::fromJson(requestJson);
        return execute(request);
    } catch (const std::exception& e) {
        return ToolResult::error(std::string("解析请求失败: ") + e.what());
    }
}

ToolResult ToolExecutor::execute(const std::string& toolName, const json& parameters) {
    ToolExecutionRequest request;
    request.tool_name = toolName;
    request.parameters = parameters;
    return execute(request);
}

std::vector<ToolDescription> ToolExecutor::getAllToolDescriptions() const {
    return registry_.getAllToolDescriptions();
}

json ToolExecutor::getToolsSchema() const {
    json schema = json::array();

    auto descriptions = getAllToolDescriptions();
    for (const auto& desc : descriptions) {
        json toolJson;
        toolJson["name"] = desc.name;
        toolJson["description"] = desc.description;

        // 构建参数Schema
        json parameters = json::object();
        parameters["type"] = "object";

        json props = json::object();
        json required = json::array();

        for (const auto& param : desc.parameters) {
            json paramDef;
            paramDef["description"] = param.description;
            paramDef["type"] = param.type;

            if (!param.default_value.empty()) {
                if (param.type == "integer") {
                    paramDef["default"] = std::stoi(param.default_value);
                } else if (param.type == "boolean") {
                    paramDef["default"] = (param.default_value == "true");
                } else {
                    paramDef["default"] = param.default_value;
                }
            }

            props[param.name] = paramDef;

            if (param.required) {
                required.push_back(param.name);
            }
        }

        parameters["properties"] = props;
        if (!required.empty()) {
            parameters["required"] = required;
        }

        toolJson["input_schema"] = parameters;
        schema.push_back(toolJson);
    }

    return schema;
}

bool ToolExecutor::hasTool(const std::string& name) const {
    return registry_.hasTool(name);
}

std::shared_ptr<ToolBase> ToolExecutor::getTool(const std::string& name) {
    return registry_.getTool(name);
}

void ToolExecutor::loadToolSettings(const ConfigManager& config_mgr) {
    const auto& config = config_mgr.getConfig();

    // 配置Bash工具的超时和禁止命令
    auto bashTool = std::dynamic_pointer_cast<BashTool>(getTool("bash"));
    if (bashTool) {
        bashTool->setTimeout(config.tools.bash_timeout);
        bashTool->setForbiddenCommands(config.tools.forbidden_commands);
    }

    // 可以添加更多工具的配置...
}

} // namespace roboclaw
