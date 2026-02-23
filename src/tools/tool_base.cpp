// ToolBase实现 - Improved error handling

#include "tool_base.h"
#include <stdexcept>
#include <shared_mutex>
#include <json/json.h>
#include <typeinfo>

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
    } catch (const json::type_error& e) {
        LOG_WARNING("Parameter '" + name + "' type error: " + std::string(e.what()));
        return defaultVal;
    } catch (const std::exception& e) {
        LOG_WARNING("Parameter '" + name + "' access error: " + std::string(e.what()));
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
    } catch (const json::type_error& e) {
        // Try converting from string
        try {
            std::string strVal = params[name].get<std::string>();
            return std::stoi(strVal);
        } catch (const json::type_error& e2) {
            LOG_WARNING("Parameter '" + name + "' type error: " + std::string(e2.what()));
            return defaultVal;
        } catch (const std::invalid_argument& e2) {
            LOG_WARNING("Parameter '" + name + "' invalid integer: " + std::string(e2.what()));
            return defaultVal;
        } catch (const std::out_of_range& e2) {
            LOG_WARNING("Parameter '" + name + "' out of range: " + std::string(e2.what()));
            return defaultVal;
        }
    } catch (const std::exception& e) {
        LOG_WARNING("Parameter '" + name + "' access error: " + std::string(e.what()));
        return defaultVal;
    }
}

bool ToolBase::getBoolParam(const json& params, const std::string& name,
                            bool defaultVal) const {
    if (!params.contains(name) || params[name].is_null()) {
        return defaultVal;
    }
    try {
        return params[name].get<bool>();
    } catch (const json::type_error& e) {
        // Try converting from string
        try {
            std::string strVal = params[name].get<std::string>();
            return (strVal == "true" || strVal == "1" || strVal == "yes");
        } catch (const json::type_error& e2) {
            LOG_WARNING("Parameter '" + name + "' type error: " + std::string(e2.what()));
            return defaultVal;
        } catch (const std::exception& e2) {
            LOG_WARNING("Parameter '" + name + "' bool conversion error: " + std::string(e2.what()));
            return defaultVal;
        }
    } catch (const std::exception& e) {
        LOG_WARNING("Parameter '" + name + "' access error: " + std::string(e.what()));
        return defaultVal;
    }
}

// ToolRegistry实现
ToolRegistry& ToolRegistry::getInstance() {
    static ToolRegistry instance;
    return instance;
}

void ToolRegistry::registerTool(const std::string& name, std::shared_ptr<ToolBase> tool) {
    std::unique_lock<std::shared_mutex> lock(tools_mutex_);
    tools_[name] = tool;
    LOG_DEBUG("工具已注册: " + name);
}

std::shared_ptr<ToolBase> ToolRegistry::getTool(const std::string& name) {
    std::shared_lock<std::shared_mutex> lock(tools_mutex_);
    auto it = tools_.find(name);
    if (it != tools_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<ToolDescription> ToolRegistry::getAllToolDescriptions() const {
    std::shared_lock<std::shared_mutex> lock(tools_mutex_);
    std::vector<ToolDescription> descriptions;
    for (const auto& pair : tools_) {
        descriptions.push_back(pair.second->getToolDescription());
    }
    return descriptions;
}

std::vector<std::string> ToolRegistry::getAllToolNames() const {
    std::shared_lock<std::shared_mutex> lock(tools_mutex_);
    std::vector<std::string> names;
    for (const auto& pair : tools_) {
        names.push_back(pair.first);
    }
    return names;
}

bool ToolRegistry::hasTool(const std::string& name) const {
    std::shared_lock<std::shared_mutex> lock(tools_mutex_);
    return tools_.find(name) != tools_.end();
}

} // namespace roboclaw
