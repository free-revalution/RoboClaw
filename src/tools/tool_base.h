// 工具基类 - ToolBase
// 所有工具的抽象基类

#ifndef ROBOCLAW_TOOLS_TOOL_BASE_H
#define ROBOCLAW_TOOLS_TOOL_BASE_H

#include <string>
#include <map>
#include <functional>
#include "../utils/logger.h"

// 使用nlohmann/json处理参数
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace roboclaw {

// 工具执行结果
struct ToolResult {
    bool success;
    std::string content;
    std::string error;
    json metadata;  // 额外元数据

    ToolResult() : success(false) {}

    static ToolResult ok(const std::string& content, const json& metadata = json::object()) {
        ToolResult result;
        result.success = true;
        result.content = content;
        result.metadata = metadata;
        return result;
    }

    static ToolResult error(const std::string& error) {
        ToolResult result;
        result.success = false;
        result.error = error;
        return result;
    }

    json toJson() const {
        json j;
        j["success"] = success;
        if (success) {
            j["content"] = content;
            if (!metadata.empty()) {
                j["metadata"] = metadata;
            }
        } else {
            j["error"] = error;
        }
        return j;
    }
};

// 工具参数描述
struct ToolParam {
    std::string name;
    std::string type;      // "string", "integer", "boolean"
    std::string description;
    bool required;
    std::string default_value;  // 默认值（字符串形式）

    json toJson() const {
        json j;
        j["name"] = name;
        j["type"] = type;
        j["description"] = description;
        j["required"] = required;
        if (!default_value.empty()) {
            j["default"] = default_value;
        }
        return j;
    }
};

// 工具描述
struct ToolDescription {
    std::string name;
    std::string description;
    std::vector<ToolParam> parameters;

    json toJson() const {
        json j;
        j["name"] = name;
        j["description"] = description;

        json params = json::array();
        for (const auto& param : parameters) {
            params.push_back(param.toJson());
        }
        j["parameters"] = params;

        return j;
    }
};

// 工具基类
class ToolBase {
public:
    ToolBase(const std::string& name, const std::string& description)
        : name_(name), description_(description) {}

    virtual ~ToolBase() = default;

    // 获取工具名称
    std::string getName() const { return name_; }

    // 获取工具描述
    std::string getDescription() const { return description_; }

    // 获取工具描述（JSON格式）
    virtual ToolDescription getDescription() const = 0;

    // 验证参数
    virtual bool validateParams(const json& params) const;

    // 执行工具
    virtual ToolResult execute(const json& params) = 0;

protected:
    std::string name_;
    std::string description_;

    // 检查必需参数
    bool hasRequiredParam(const json& params, const std::string& paramName) const;

    // 获取字符串参数
    std::string getStringParam(const json& params, const std::string& name,
                               const std::string& defaultVal = "") const;

    // 获取整数参数
    int getIntParam(const json& params, const std::string& name,
                    int defaultVal = 0) const;

    // 获取布尔参数
    bool getBoolParam(const json& params, const std::string& name,
                      bool defaultVal = false) const;
};

// 工具注册表
class ToolRegistry {
public:
    static ToolRegistry& getInstance();

    // 注册工具
    void registerTool(const std::string& name, std::shared_ptr<ToolBase> tool);

    // 获取工具
    std::shared_ptr<ToolBase> getTool(const std::string& name);

    // 获取所有工具描述
    std::vector<ToolDescription> getAllToolDescriptions() const;

    // 获取所有工具名称
    std::vector<std::string> getAllToolNames() const;

    // 检查工具是否存在
    bool hasTool(const std::string& name) const;

private:
    ToolRegistry() = default;

    std::map<std::string, std::shared_ptr<ToolBase>> tools_;
};

} // namespace roboclaw

#endif // ROBOCLAW_TOOLS_TOOL_BASE_H
