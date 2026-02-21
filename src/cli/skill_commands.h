// 技能CLI命令 - SkillCommands
// 提供技能管理相关的命令行接口

#ifndef ROBOCLAW_CLI_SKILL_COMMANDS_H
#define ROBOCLAW_CLI_SKILL_COMMANDS_H

#include "../skills/skill_registry.h"
#include "../storage/config_manager.h"
#include <memory>
#include <string>

namespace roboclaw {

// 技能命令处理器
class SkillCommands {
public:
    SkillCommands(std::shared_ptr<SkillRegistry> registry,
                  const ConfigManager& config);
    ~SkillCommands() = default;

    // 列出所有技能
    int listSkills();

    // 显示技能详情
    int showSkill(const std::string& skillName);

    // 安装技能（从本地文件或远程URL）
    int installSkill(const std::string& source, const std::string& url = "");

    // 卸载技能
    int uninstallSkill(const std::string& skillName);

    // 更新技能
    int updateSkill(const std::string& skillName);

    // 创建新技能模板
    int createSkill(const std::string& skillName);

    // 重新加载所有技能
    int reloadSkills();

    // 执行技能
    int executeSkill(const std::string& skillName, const std::vector<std::string>& args);

    // 搜索技能
    int searchSkills(const std::string& keyword);

private:
    // 获取技能目录
    std::string getSkillsDir() const;

    // 获取内置技能目录
    std::string getBuiltinSkillsDir() const;

    // 获取用户技能目录
    std::string getUserSkillsDir() const;

    // 显示技能信息
    void displaySkillInfo(const Skill& skill);

    // 解析技能参数
    std::map<std::string, std::string> parseSkillArgs(
        const Skill& skill,
        const std::vector<std::string>& args);

    std::shared_ptr<SkillRegistry> registry_;
    const ConfigManager& config_;
};

} // namespace roboclaw

#endif // ROBOCLAW_CLI_SKILL_COMMANDS_H
