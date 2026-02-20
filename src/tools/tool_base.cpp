// ToolBase实现

#include "tool_base.h"
#include <stdexcept>

namespace roboclaw {

// ToolBase实现
bool ToolBase::validateParams(const json& params) const {
    // 默认实现：子类可以覆盖
    return true;
}

bool ToolBase::hasRequiredParam(const json& params, const std::string& paramName) const {
    return params.contains(paramName) && !params[paramName].is_null();
}

std::string ToolBase::getStringParam(const json& params, const std::string& name,
                                     const std::string& defaultVal) const {
    if (!params.contains(name) || params[name].is_null()) {
        return defaultVal;
    }
    try {
        return params[name].get<std::string>();
    } catch (...) {
        return defaultVal;
    }
}

int ToolBase::getIntParam(const json& params, const std::string& name,
                          int defaultVal) const {
    if (!params.contains(name) || params[name].is_null()) {
        return defaultVal;
    }
    try {
        return params[name].get<int>();
    } catch (...) {
        try {
            // 尝试从字符串转换
            std::string strVal = params[name].get<std::string>();
            return std::stoi(strVal);
        } catch (...) {
            return defaultVal;
        }
    }
}

bool ToolBase::getBoolParam(const json& params, const std::string& name,
                            bool defaultVal) const {
    if (!params.contains(name) || params[name].is_null()) {
        return defaultVal;
    }
    try {
        return params[name].get<bool>();
    } catch (...) {
        try {
            // 尝试从字符串转换
            std::string strVal = params[name].get<std::string>();
            return (strVal == "true" || strVal == "1" || strVal == "yes");
        } catch (...) {
            return defaultVal;
        }
    }
}

// ToolRegistry实现
ToolRegistry& ToolRegistry::getInstance() {
    static ToolRegistry instance;
    return instance;
}

void ToolRegistry::registerTool(const std::string& name, std::shared_ptr<ToolBase> tool) {
    tools_[name] = tool;
    LOG_DEBUG("工具已注册: " + name);
}

std::shared_ptr<ToolBase> ToolRegistry::getTool(const std::string& name) {
    auto it = tools_.find(name);
    if (it != tools_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<ToolDescription> ToolRegistry::getAllToolDescriptions() const {
    std::vector<ToolDescription> descriptions;
    for (const auto& pair : tools_) {
        descriptions.push_back(pair.second->getDescription());
    }
    return descriptions;
}

std::vector<std::string> ToolRegistry::getAllToolNames() const {
    std::vector<std::string> names;
    for (const auto& pair : tools_) {
        names.push_back(pair.first);
    }
    return names;
}

bool ToolRegistry::hasTool(const std::string& name) const {
    return tools_.find(name) != tools_.end();
}

} // namespace roboclaw
