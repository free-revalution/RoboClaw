// 技能注册表 - SkillRegistry
// 管理所有加载的技能

#ifndef ROBOCLAW_SKILLS_SKILL_REGISTRY_H
#define ROBOCLAW_SKILLS_SKILL_REGISTRY_H

#include "skill_parser.h"
#include <string>
#include <map>
#include <memory>
#include <vector>
#include <functional>
#include <shared_mutex>

namespace roboclaw {

// 技能注册表
class SkillRegistry {
public:
    SkillRegistry();
    ~SkillRegistry() = default;

    // 注册技能
    bool registerSkill(const Skill& skill);
    bool unregisterSkill(const std::string& name);

    // 获取技能
    std::shared_ptr<Skill> getSkill(const std::string& name) const;

    // 检查技能是否存在
    bool hasSkill(const std::string& name) const;

    // 获取所有技能
    std::vector<std::shared_ptr<Skill>> getAllSkills() const;
    std::vector<std::string> getSkillNames() const;

    // 根据触发器匹配技能
    std::vector<std::shared_ptr<Skill>> matchSkills(const std::string& input) const;

    // 加载技能目录
    int loadSkillsFromDirectory(const std::string& directory);

    // 重新加载所有技能
    void reloadAll();

    // 获取技能数量
    size_t getSkillCount() const { return skills_.size(); }

    // 设置技能变化回调
    void setChangeCallback(std::function<void(const std::string&, bool)> callback) {
        change_callback_ = callback;
    }

private:
    std::map<std::string, std::shared_ptr<Skill>> skills_;
    std::map<std::string, std::string> skill_filepaths_;  // name -> filepath

    SkillParser parser_;

    std::function<void(const std::string&, bool)> change_callback_;

    // 读写锁保证线程安全
    mutable std::shared_mutex skills_mutex_;

    // 通知技能变化
    void notifyChange(const std::string& name, bool added);

    // 解析技能文件
    bool loadSkillFile(const std::string& filepath);

    // 扫描目录中的技能文件
    std::vector<std::string> scanSkillFiles(const std::string& directory) const;

    // 判断是否是技能文件
    bool isSkillFile(const std::string& filename) const;
};

} // namespace roboclaw

#endif // ROBOCLAW_SKILLS_SKILL_REGISTRY_H
