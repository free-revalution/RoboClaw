// 技能执行器 - SkillExecutor
// 在Agent层执行技能操作

#ifndef ROBOCLAW_SKILLS_SKILL_EXECUTOR_H
#define ROBOCLAW_SKILLS_SKILL_EXECUTOR_H

#include "skill_parser.h"
#include "../agent/agent.h"
#include "../tools/tool_base.h"
#include <string>
#include <functional>
#include <memory>

namespace roboclaw {

// 技能执行上下文
struct SkillExecutionContext {
    std::string user_input;           // 用户输入
    std::vector<ChatMessage> history; // 对话历史
    std::shared_ptr<Agent> agent;     // Agent实例
    std::map<std::string, std::string> variables; // 变量绑定

    // 获取变量
    std::string getVariable(const std::string& name, const std::string& defaultVal = "") const {
        auto it = variables.find(name);
        return (it != variables.end()) ? it->second : defaultVal;
    }
};

// 技能执行结果
struct SkillExecutionResult {
    bool success;              // 是否成功
    std::string output;        // 输出内容
    std::string error;         // 错误信息
    std::vector<ToolResult> tool_results; // 工具调用结果

    SkillExecutionResult()
        : success(false) {}
};

// 技能执行器
class SkillExecutor {
public:
    SkillExecutor(std::shared_ptr<ToolExecutor> toolMgr,
                  std::shared_ptr<Agent> agent);
    ~SkillExecutor() = default;

    // 执行技能
    SkillExecutionResult execute(const Skill& skill,
                                  const SkillExecutionContext& context);

    // 执行单个动作
    SkillExecutionResult executeAction(const SkillAction& action,
                                        const SkillExecutionContext& context);

    // 设置LLM调用回调
    using LLMCallback = std::function<std::string(const std::string& prompt,
                                                   const std::vector<ChatMessage>& history)>;
    void setLLMCallback(LLMCallback callback) { llm_callback_ = callback; }

private:
    // 执行工具动作
    SkillExecutionResult executeToolAction(const SkillAction& action,
                                            const SkillExecutionContext& context);

    // 执行LLM动作
    SkillExecutionResult executeLLMAction(const SkillAction& action,
                                          const SkillExecutionContext& context);

    // 执行脚本动作
    SkillExecutionResult executeScriptAction(const SkillAction& action,
                                              const SkillExecutionContext& context);

    // 替换模板变量
    std::string replaceTemplateVariables(const std::string& tmpl,
                                          const SkillExecutionContext& context);

    // 解析参数
    json parseParameters(const json& params,
                          const SkillExecutionContext& context);

    std::shared_ptr<ToolExecutor> tool_manager_;
    std::shared_ptr<Agent> agent_;
    LLMCallback llm_callback_;
};

} // namespace roboclaw

#endif // ROBOCLAW_SKILLS_SKILL_EXECUTOR_H
