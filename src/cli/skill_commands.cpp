// SkillCommands实现

#include "skill_commands.h"
#include "../utils/logger.h"
#include "../skills/skill_parser.h"
#include <iostream>
#include <filesystem>
#include <algorithm>

namespace roboclaw {

SkillCommands::SkillCommands(std::shared_ptr<SkillRegistry> registry,
                              const ConfigManager& config)
    : registry_(registry)
    , config_(config) {
}

std::string SkillCommands::getSkillsDir() const {
    // 从配置读取，如果没有则使用默认值
    std::string skillsDir = config_.get("skills.local_skills_dir", "~/.roboclaw/skills");

    // 展开波浪号
    if (skillsDir[0] == '~') {
        const char* home = std::getenv("HOME");
        if (home) {
            skillsDir = std::string(home) + skillsDir.substr(1);
        }
    }

    return skillsDir;
}

std::string SkillCommands::getBuiltinSkillsDir() const {
    return "skills/builtin";
}

std::string SkillCommands::getUserSkillsDir() const {
    return getSkillsDir() + "/user";
}

int SkillCommands::listSkills() {
    auto skills = registry_->getAllSkills();

    if (skills.empty()) {
        std::cout << "没有已安装的技能。" << std::endl;
        return 0;
    }

    std::cout << "已安装的技能 (" << skills.size() << "):" << std::endl;
    std::cout << std::string(60, '-') << std::endl;

    // 按名称排序
    std::sort(skills.begin(), skills.end(),
        [](const auto& a, const auto& b) {
            return a->name < b->name;
        });

    for (const auto& skill : skills) {
        std::cout << "  " << skill->name;
        if (!skill->version.empty()) {
            std::cout << " (" << skill->version << ")";
        }
        std::cout << std::endl;

        if (!skill->description.empty()) {
            std::cout << "    " << skill->description << std::endl;
        }

        // 显示触发词
        if (!skill->triggers.empty()) {
            std::cout << "    触发词: ";
            for (size_t i = 0; i < skill->triggers.size() && i < 3; ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << "\"" << skill->triggers[i] << "\"";
            }
            if (skill->triggers.size() > 3) {
                std::cout << " ...";
            }
            std::cout << std::endl;
        }

        std::cout << std::endl;
    }

    return 0;
}

int SkillCommands::showSkill(const std::string& skillName) {
    auto skill = registry_->getSkill(skillName);

    if (!skill) {
        std::cerr << "错误: 技能不存在: " << skillName << std::endl;
        return 1;
    }

    displaySkillInfo(*skill);
    return 0;
}

void SkillCommands::displaySkillInfo(const Skill& skill) {
    std::cout << "技能名称: " << skill.name << std::endl;

    if (!skill.version.empty()) {
        std::cout << "版本: " << skill.version << std::endl;
    }

    if (!skill.author.empty()) {
        std::cout << "作者: " << skill.author << std::endl;
    }

    if (!skill.description.empty()) {
        std::cout << "描述: " << skill.description << std::endl;
    }

    if (!skill.triggers.empty()) {
        std::cout << "触发词:" << std::endl;
        for (const auto& trigger : skill.triggers) {
            std::cout << "  - " << trigger << std::endl;
        }
    }

    if (!skill.actions.empty()) {
        std::cout << "动作 (" << skill.actions.size() << "):" << std::endl;
        for (size_t i = 0; i < skill.actions.size(); ++i) {
            const auto& action = skill.actions[i];
            std::cout << "  " << (i + 1) << ". ";

            switch (action.type) {
                case ActionType::TOOL:
                    std::cout << "[工具] " << action.name;
                    break;
                case ActionType::LLM:
                    std::cout << "[LLM] ";
                    break;
                case ActionType::SCRIPT:
                    std::cout << "[脚本] ";
                    break;
                case ActionType::CUSTOM:
                    std::cout << "[自定义] ";
                    break;
            }

            if (!action.description.empty()) {
                std::cout << " - " << action.description;
            }
            std::cout << std::endl;
        }
    }

    if (skill.parameters.contains("properties")) {
        std::cout << "参数:" << std::endl;
        // 简化显示，实际应该解析properties
        std::cout << "  (参数定义)" << std::endl;
    }
}

int SkillCommands::installSkill(const std::string& source, const std::string& url) {
    std::string destPath;

    if (url.empty()) {
        // 从本地文件安装
        std::filesystem::path sourcePath(source);

        if (!std::filesystem::exists(sourcePath)) {
            std::cerr << "错误: 文件不存在: " << source << std::endl;
            return 1;
        }

        // 复制到用户技能目录
        std::string userDir = getUserSkillsDir();
        std::filesystem::create_directories(userDir);

        destPath = userDir + "/" + sourcePath.filename().string();

        try {
            std::filesystem::copy_file(sourcePath, destPath,
                std::filesystem::copy_options::overwrite_existing);
        } catch (const std::exception& e) {
            std::cerr << "错误: 复制文件失败: " << e.what() << std::endl;
            return 1;
        }

    } else {
        // TODO: 从URL下载（需要SkillDownloader）
        std::cerr << "错误: 从远程下载功能尚未实现" << std::endl;
        return 1;
    }

    // 解析并注册技能
    SkillParser parser;
    Skill skill;

    if (parser.parseFile(destPath, skill)) {
        if (registry_->registerSkill(skill)) {
            std::cout << "技能已安装: " << skill.name << std::endl;
            return 0;
        } else {
            std::cerr << "错误: 技能注册失败" << std::endl;
            return 1;
        }
    } else {
        std::cerr << "错误: 无法解析技能文件" << std::endl;
        return 1;
    }
}

int SkillCommands::uninstallSkill(const std::string& skillName) {
    if (!registry_->hasSkill(skillName)) {
        std::cerr << "错误: 技能不存在: " << skillName << std::endl;
        return 1;
    }

    if (registry_->unregisterSkill(skillName)) {
        std::cout << "技能已卸载: " << skillName << std::endl;

        // TODO: 删除技能文件（需要记录文件路径）
        return 0;
    }

    std::cerr << "错误: 卸载失败" << std::endl;
    return 1;
}

int SkillCommands::updateSkill(const std::string& skillName) {
    // TODO: 实现技能更新
    std::cout << "技能更新功能尚未实现" << std::endl;
    return 1;
}

int SkillCommands::createSkill(const std::string& skillName) {
    // 创建技能模板
    std::string userDir = getUserSkillsDir();
    std::filesystem::create_directories(userDir);

    std::string filename = userDir + "/" + skillName + ".skill";

    // 检查是否已存在
    if (std::filesystem::exists(filename)) {
        std::cerr << "错误: 技能文件已存在: " << filename << std::endl;
        return 1;
    }

    // 创建模板内容
    std::string templateContent = R"({
  "name": ")" + skillName + R"(",
  "description": "技能描述",
  "version": "1.0.0",
  "author": "",
  "triggers": [
    "触发词1",
    "触发词2"
  ],
  "parameters": {
    "param1": {
      "type": "string",
      "description": "参数描述",
      "required": true
    }
  },
  "actions": [
    {
      "type": "tool",
      "name": "read",
      "description": "操作描述",
      "parameters": {
        "file": "${param1}"
      }
    }
  ]
}
)";

    try {
        std::ofstream file(filename);
        file << templateContent;
        file.close();

        std::cout << "已创建技能模板: " << filename << std::endl;
        std::cout << "请编辑文件以完善技能定义" << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "错误: 创建文件失败: " << e.what() << std::endl;
        return 1;
    }
}

int SkillCommands::reloadSkills() {
    std::string builtinDir = getBuiltinSkillsDir();
    std::string userDir = getUserSkillsDir();

    int loaded = 0;

    // 加载内置技能
    if (std::filesystem::exists(builtinDir)) {
        loaded += registry_->loadSkillsFromDirectory(builtinDir);
    }

    // 加载用户技能
    if (std::filesystem::exists(userDir)) {
        loaded += registry_->loadSkillsFromDirectory(userDir);
    }

    std::cout << "已重新加载 " << loaded << " 个技能" << std::endl;
    return 0;
}

int SkillCommands::executeSkill(const std::string& skillName,
                                 const std::vector<std::string>& args) {
    auto skill = registry_->getSkill(skillName);

    if (!skill) {
        std::cerr << "错误: 技能不存在: " << skillName << std::endl;
        return 1;
    }

    // 解析参数
    auto params = parseSkillArgs(*skill, args);

    // TODO: 创建执行上下文并执行
    std::cout << "执行技能: " << skillName << std::endl;
    std::cout << "功能待实现..." << std::endl;

    return 0;
}

std::map<std::string, std::string> SkillCommands::parseSkillArgs(
    const Skill& skill,
    const std::vector<std::string>& args) {

    std::map<std::string, std::string> params;

    // 简化实现：按顺序填充参数
    // 实际应该解析 --key=value 格式

    return params;
}

int SkillCommands::searchSkills(const std::string& keyword) {
    auto skills = registry_->getAllSkills();

    std::vector<std::shared_ptr<Skill>> matched;

    for (const auto& skill : skills) {
        // 搜索名称、描述、触发词
        std::string searchContent = skill->name + " " +
                                    skill->description + " ";
        for (const auto& trigger : skill->triggers) {
            searchContent += trigger + " ";
        }

        // 转小写
        std::string lowerContent = searchContent;
        std::string lowerKeyword = keyword;
        std::transform(lowerContent.begin(), lowerContent.end(),
                       lowerContent.begin(), ::tolower);
        std::transform(lowerKeyword.begin(), lowerKeyword.end(),
                       lowerKeyword.begin(), ::tolower);

        if (lowerContent.find(lowerKeyword) != std::string::npos) {
            matched.push_back(skill);
        }
    }

    if (matched.empty()) {
        std::cout << "未找到匹配 \"" << keyword << "\" 的技能" << std::endl;
        return 0;
    }

    std::cout << "找到 " << matched.size() << " 个匹配的技能:" << std::endl;
    std::cout << std::string(60, '-') << std::endl;

    for (const auto& skill : matched) {
        std::cout << "  " << skill->name;
        if (!skill->description.empty()) {
            std::cout << " - " << skill->description;
        }
        std::cout << std::endl;
    }

    return 0;
}

} // namespace roboclaw
