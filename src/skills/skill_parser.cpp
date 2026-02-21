// SkillParser实现

#include "skill_parser.h"
#include "../utils/logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>

namespace roboclaw {

SkillParser::SkillParser() {
}

bool SkillParser::parseFile(const std::string& filepath, Skill& skill) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        LOG_ERROR("无法打开技能文件: " + filepath);
        return false;
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();

    // 检测格式
    std::string format = detectFormat(filepath);
    if (format == "auto") {
        if (filepath.find(".json") != std::string::npos) {
            format = "json";
        } else if (filepath.find(".yaml") != std::string::npos ||
                   filepath.find(".yml") != std::string::npos) {
            format = "yaml";
        } else {
            // 根据内容判断
            if (content.find("{") == 0 || content.find("[") == 0) {
                format = "json";
            } else {
                format = "yaml";
            }
        }
    }

    skill.file_path = filepath;
    return parseContent(content, skill, format);
}

bool SkillParser::parseContent(const std::string& content, Skill& skill, const std::string& format) {
    if (format == "yaml") {
        return parseYaml(content, skill);
    } else if (format == "json") {
        return parseJsonContent(content, skill);
    } else {
        // 自动检测
        if (content.find("{") == 0 || content.find("[") == 0) {
            return parseJsonContent(content, skill);
        } else {
            return parseYaml(content, skill);
        }
    }
}

bool SkillParser::parseJson(const json& j, Skill& skill) {
    try {
        skill = Skill::fromJson(j);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("解析技能JSON失败: " + std::string(e.what()));
        return false;
    }
}

bool SkillParser::saveToFile(const Skill& skill, const std::string& filepath) {
    try {
        // 创建目录
        size_t last_slash = filepath.find_last_of('/');
        if (last_slash != std::string::npos) {
            std::string dir = filepath.substr(0, last_slash);
            std::filesystem::create_directories(dir);
        }

        std::ofstream file(filepath);
        if (!file.is_open()) {
            LOG_ERROR("无法创建技能文件: " + filepath);
            return false;
        }

        json skill_json = skill.toJson();
        file << skill_json.dump(2);
        file.close();

        LOG_DEBUG("技能已保存: " + filepath);
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("保存技能文件失败: " + std::string(e.what()));
        return false;
    }
}

std::string SkillParser::detectFormat(const std::string& filepath) const {
    if (filepath.find(".json") != std::string::npos) {
        return "json";
    } else if (filepath.find(".yaml") != std::string::npos ||
               filepath.find(".yml") != std::string::npos) {
        return "yaml";
    } else if (filepath.find(".skill") != std::string::npos) {
        return "yaml";  // 默认.skill是YAML
    }
    return "auto";
}

bool SkillParser::parseYaml(const std::string& content, Skill& skill) {
    std::istringstream stream(content);
    std::string line;
    std::string current_section;

    while (std::getline(stream, line)) {
        // 去除BOM和空白字符
        line.erase(0, line.find_first_not_of("\r\n\t "));
        line.erase(line.find_last_not_of("\r\n\t ") + 1);

        if (line.empty() || line[0] == '#') {
            continue;  // 跳过空行和注释
        }

        // 检测节（key: value或key:）
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = line.substr(0, colon_pos);
            std::string value = line.substr(colon_pos + 1);

            // 去除前后空格
            key = std::regex_replace(key, std::regex("^\\s+|\\s+$"), "");
            value = std::regex_replace(value, std::regex("^\\s+|\\s+$"), "");

            // 处理不同的节
            if (key == "name") {
                skill.name = value;
            } else if (key == "description") {
                skill.description = value;
            } else if (key == "version") {
                skill.version = value;
            } else if (key == "author") {
                skill.author = value;
            } else if (key == "triggers") {
                // 简化处理：假设下一行是列表项
                // 实际应用中需要完整的YAML解析器
            } else if (key == "actions") {
                // 动作列表 - 需要完整解析
            } else if (key == "parameters") {
                // 参数定义
            }
        }
    }

    // 简化实现：对于YAML，我们这里只做基础解析
    // 完整实现需要集成yaml-cpp库

    skill.is_builtin = false;
    return !skill.name.empty();
}

bool SkillParser::parseJsonContent(const std::string& content, Skill& skill) {
    try {
        json j = json::parse(content);
        return parseJson(j, skill);
    } catch (const std::exception& e) {
        LOG_ERROR("解析技能JSON失败: " + std::string(e.what()));
        return false;
    }
}

std::string SkillParser::extractYamlValue(const std::string& content, const std::string& key) {
    // 简化的YAML值提取
    std::string pattern = key + ":\\s*(.+)";
    std::regex regex(pattern);
    std::smatch match;

    if (std::regex_search(content, match, regex)) {
        return match[1].str();
    }

    return "";
}

bool Skill::matchesTrigger(const std::string& input) const {
    std::string input_lower = input;
    std::transform(input_lower.begin(), input_lower.end(), input_lower.begin(), ::tolower);

    for (const auto& trigger : triggers) {
        std::string trigger_lower = trigger;
        std::transform(trigger_lower.begin(), trigger_lower.end(), trigger_lower.begin(), ::tolower);

        if (input_lower.find(trigger_lower) != std::string::npos) {
            return true;
        }
    }

    return false;
}

} // namespace roboclaw
