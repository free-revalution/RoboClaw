// 技能文件解析器 - SkillParser
// 解析.skill文件（YAML/JSON格式）

#ifndef ROBOCLAW_SKILLS_SKILL_PARSER_H
#define ROBOCLAW_SKILLS_SKILL_PARSER_H

#include <string>
#include <vector>
#include <map>
#include <memory>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace roboclaw {

// 技能动作类型
enum class ActionType {
    TOOL,    // 调用工具
    LLM,     // 调用LLM
    SCRIPT,  // 执行脚本
    CUSTOM   // 自定义操作
};

// 技能动作
struct SkillAction {
    ActionType type;
    std::string name;
    std::string description;
    json parameters;

    // 对于LLM类型：prompt模板
    std::string prompt_template;

    // 对于SCRIPT类型：命令列表
    std::vector<std::string> commands;

    json toJson() const {
        json j;
        j["type"] = static_cast<int>(type);
        j["name"] = name;
        j["description"] = description;
        j["parameters"] = parameters;

        if (type == ActionType::LLM) {
            j["prompt_template"] = prompt_template;
        } else if (type == ActionType::SCRIPT) {
            j["commands"] = commands;
        }

        return j;
    }

    static SkillAction fromJson(const json& j) {
        SkillAction action;
        action.type = static_cast<ActionType>(j.value("type", 0));
        action.name = j.value("name", "");
        action.description = j.value("description", "");
        action.parameters = j.value("parameters", json::object());

        if (action.type == ActionType::LLM) {
            action.prompt_template = j.value("prompt_template", "");
        } else if (action.type == ActionType::SCRIPT) {
            if (j.contains("commands")) {
                for (const auto& cmd : j["commands"]) {
                    action.commands.push_back(cmd.get<std::string>());
                }
            }
        }

        return action;
    }
};

// 技能定义
struct Skill {
    std::string name;
    std::string description;
    std::string version;
    std::string author;

    // 触发器
    std::vector<std::string> triggers;

    // 动作序列
    std::vector<SkillAction> actions;

    // 参数定义
    json parameters;

    // 元数据
    std::string file_path;  // 来源文件路径
    bool is_builtin;      // 是否内置技能

    Skill() : is_builtin(false) {}

    json toJson() const {
        json j;
        j["name"] = name;
        j["description"] = description;
        j["version"] = version;
        j["author"] = author;
        j["triggers"] = triggers;

        json actions_json = json::array();
        for (const auto& action : actions) {
            actions_json.push_back(action.toJson());
        }
        j["actions"] = actions_json;

        j["parameters"] = parameters;
        j["file_path"] = file_path;
        j["is_builtin"] = is_builtin;

        return j;
    }

    static Skill fromJson(const json& j) {
        Skill skill;
        skill.name = j.value("name", "");
        skill.description = j.value("description", "");
        skill.version = j.value("version", "1.0.0");
        skill.author = j.value("author", "");
        skill.file_path = j.value("file_path", "");
        skill.is_builtin = j.value("is_builtin", false);

        if (j.contains("triggers")) {
            for (const auto& trigger : j["triggers"]) {
                skill.triggers.push_back(trigger.get<std::string>());
            }
        }

        if (j.contains("actions")) {
            for (const auto& action_json : j["actions"]) {
                skill.actions.push_back(SkillAction::fromJson(action_json));
            }
        }

        skill.parameters = j.value("parameters", json::object());

        return skill;
    }

    // 检查触发器是否匹配
    bool matchesTrigger(const std::string& input) const;
};

// 技能解析器
class SkillParser {
public:
    SkillParser();
    ~SkillParser() = default;

    // 解析技能文件
    bool parseFile(const std::string& filepath, Skill& skill);

    // 解析技能内容
    bool parseContent(const std::string& content, Skill& skill, const std::string& format = "auto");

    // 从JSON解析
    bool parseJson(const json& j, Skill& skill);

    // 保存技能到文件
    bool saveToFile(const Skill& skill, const std::string& filepath);

    // 检测文件格式
    std::string detectFormat(const std::string& filepath) const;

private:
    // 解析YAML格式（简化实现）
    bool parseYaml(const std::string& content, Skill& skill);

    // 解析JSON格式
    bool parseJsonContent(const std::string& content, Skill& skill);

    // 提取YAML值
    std::string extractYamlValue(const std::string& content, const std::string& key);
};

} // namespace roboclaw

#endif // ROBOCLAW_SKILLS_SKILL_PARSER_H
