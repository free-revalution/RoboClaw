// SkillRegistry实现

#include "skill_registry.h"
#include "../utils/logger.h"
#include <filesystem>
#include <algorithm>
#include <shared_mutex>

namespace roboclaw {

SkillRegistry::SkillRegistry() {
}

bool SkillRegistry::registerSkill(const Skill& skill) {
    if (skill.name.empty()) {
        LOG_ERROR("技能名称不能为空");
        return false;
    }

    std::unique_lock<std::shared_mutex> lock(skills_mutex_);

    // 检查是否已存在
    if (skills_.find(skill.name) != skills_.end()) {
        LOG_WARNING("技能已存在: " + skill.name);
        return false;
    }

    // 添加技能
    auto skill_ptr = std::make_shared<Skill>(skill);
    skills_[skill.name] = skill_ptr;

    if (!skill.file_path.empty()) {
        skill_filepaths_[skill.name] = skill.file_path;
    }

    notifyChange(skill.name, true);
    LOG_INFO("技能已注册: " + skill.name);
    return true;
}

bool SkillRegistry::unregisterSkill(const std::string& name) {
    std::unique_lock<std::shared_mutex> lock(skills_mutex_);

    auto it = skills_.find(name);
    if (it == skills_.end()) {
        LOG_WARNING("技能不存在: " + name);
        return false;
    }

    skills_.erase(it);
    skill_filepaths_.erase(name);

    notifyChange(name, false);
    LOG_INFO("技能已卸载: " + name);
    return true;
}

std::shared_ptr<Skill> SkillRegistry::getSkill(const std::string& name) const {
    std::shared_lock<std::shared_mutex> lock(skills_mutex_);
    auto it = skills_.find(name);
    if (it != skills_.end()) {
        return it->second;
    }
    return nullptr;
}

bool SkillRegistry::hasSkill(const std::string& name) const {
    std::shared_lock<std::shared_mutex> lock(skills_mutex_);
    return skills_.find(name) != skills_.end();
}

std::vector<std::shared_ptr<Skill>> SkillRegistry::getAllSkills() const {
    std::shared_lock<std::shared_mutex> lock(skills_mutex_);
    std::vector<std::shared_ptr<Skill>> skills;

    for (const auto& pair : skills_) {
        skills.push_back(pair.second);
    }

    return skills;
}

std::vector<std::string> SkillRegistry::getSkillNames() const {
    std::shared_lock<std::shared_mutex> lock(skills_mutex_);
    std::vector<std::string> names;

    for (const auto& pair : skills_) {
        names.push_back(pair.first);
    }

    std::sort(names.begin(), names.end());
    return names;
}

std::vector<std::shared_ptr<Skill>> SkillRegistry::matchSkills(const std::string& input) const {
    std::shared_lock<std::shared_mutex> lock(skills_mutex_);
    std::vector<std::shared_ptr<Skill>> matched;

    for (const auto& pair : skills_) {
        if (pair.second->matchesTrigger(input)) {
            matched.push_back(pair.second);
        }
    }

    return matched;
}

int SkillRegistry::loadSkillsFromDirectory(const std::string& directory) {
    if (!std::filesystem::exists(directory)) {
        LOG_WARNING("技能目录不存在: " + directory);
        return 0;
    }

    std::vector<std::string> skill_files = scanSkillFiles(directory);
    int loaded = 0;

    for (const auto& filepath : skill_files) {
        if (loadSkillFile(filepath)) {
            loaded++;
        }
    }

    LOG_INFO("从目录加载技能: " + directory + " (" + std::to_string(loaded) + "/" +
             std::to_string(skill_files.size()) + ")");

    return loaded;
}

void SkillRegistry::reloadAll() {
    std::vector<std::string> paths;

    {
        std::shared_lock<std::shared_mutex> lock(skills_mutex_);
        // 收集所有文件路径
        for (const auto& pair : skill_filepaths_) {
            paths.push_back(pair.second);
        }
    }

    // 清空当前技能
    {
        std::unique_lock<std::shared_mutex> lock(skills_mutex_);
        skills_.clear();
    }

    // 重新加载
    for (const auto& filepath : paths) {
        loadSkillFile(filepath);
    }

    LOG_INFO("已重新加载所有技能");
}

void SkillRegistry::notifyChange(const std::string& name, bool added) {
    if (change_callback_) {
        change_callback_(name, added);
    }
}

bool SkillRegistry::loadSkillFile(const std::string& filepath) {
    Skill skill;
    if (!parser_.parseFile(filepath, skill)) {
        LOG_WARNING("无法解析技能文件: " + filepath);
        return false;
    }

    // 如果没有名称，从文件名提取
    if (skill.name.empty()) {
        std::filesystem::path path(filepath);
        std::string filename = path.filename().string();

        // 移除扩展名
        size_t dot_pos = filename.find_last_of('.');
        if (dot_pos != std::string::npos) {
            filename = filename.substr(0, dot_pos);
        }

        // 转换为有效标识符（替换-为_）
        std::replace(filename.begin(), filename.end(), '-', '_');

        skill.name = filename;
    }

    return registerSkill(skill);
}

std::vector<std::string> SkillRegistry::scanSkillFiles(const std::string& directory) const {
    std::vector<std::string> skill_files;

    try {
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.is_regular_file() && isSkillFile(entry.path().string())) {
                skill_files.push_back(entry.path().string());
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("扫描技能目录失败: " + std::string(e.what()));
    }

    std::sort(skill_files.begin(), skill_files.end());
    return skill_files;
}

bool SkillRegistry::isSkillFile(const std::string& filename) const {
    // 检查文件扩展名
    std::string lower = filename;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    return (lower.find(".skill") != std::string::npos ||
            lower.find(".yaml") != std::string::npos ||
            lower.find(".yml") != std::string::npos ||
            lower.find(".json") != std::string::npos);
}

} // namespace roboclaw
