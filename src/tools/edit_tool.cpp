// EditTool实现

#include "edit_tool.h"
#include <sstream>
#include <algorithm>

namespace roboclaw {

EditTool::EditTool()
    : ToolBase("edit", "精确替换文件内容") {
}

ToolDescription EditTool::getDescription() const {
    ToolDescription desc;
    desc.name = name_;
    desc.description = description_ + "（包括缩进和空格）";

    desc.parameters = {
        {"path", "string", "文件路径（必需）", true, ""},
        {"old_string", "string", "要替换的内容（必需）", true, ""},
        {"new_string", "string", "替换后的内容（必需）", true, ""}
    };

    return desc;
}

bool EditTool::validateParams(const json& params) const {
    if (!hasRequiredParam(params, "path")) {
        return false;
    }
    if (!hasRequiredParam(params, "old_string")) {
        return false;
    }
    if (!hasRequiredParam(params, "new_string")) {
        return false;
    }

    std::string path = getStringParam(params, "path");
    std::string oldString = getStringParam(params, "old_string");

    if (path.empty()) {
        return false;
    }

    if (oldString.empty()) {
        return false;
    }

    // 检查文件是否存在
    if (!std::filesystem::exists(path)) {
        return false;
    }

    return true;
}

ToolResult EditTool::execute(const json& params) {
    if (!validateParams(params)) {
        return ToolResult::error("参数验证失败：path、old_string和new_string都是必需参数");
    }

    std::string path = getStringParam(params, "path");
    std::string oldString = getStringParam(params, "old_string");
    std::string newString = getStringParam(params, "new_string");

    LOG_DEBUG("编辑文件: " + path);

    // 验证old_string在文件中存在
    if (!validateOldStringExists(path, oldString)) {
        return ToolResult::error("未找到要替换的内容: " + oldString.substr(0, 50) + "...");
    }

    // 执行编辑
    std::string error;
    int replaceCount = 0;
    std::vector<int> affectedLines;

    if (!editFile(path, oldString, newString, error, replaceCount, affectedLines)) {
        return ToolResult::error(error);
    }

    // 构建元数据
    json metadata;
    metadata["path"] = path;
    metadata["replace_count"] = replaceCount;
    metadata["affected_lines"] = affectedLines;

    LOG_DEBUG("文件编辑成功: " + path + " (" + std::to_string(replaceCount) + " 处替换)");

    return ToolResult::ok("已成功编辑文件: " + path + "，替换了 " + std::to_string(replaceCount) + " 处", metadata);
}

bool EditTool::editFile(const std::string& path, const std::string& oldString,
                        const std::string& newString, std::string& error,
                        int& replaceCount, std::vector<int>& affectedLines) {
    try {
        // 读取所有行
        std::vector<std::string> lines;
        std::ifstream file(path);
        if (!file.is_open()) {
            error = "无法打开文件: " + path;
            return false;
        }

        std::string line;
        while (std::getline(file, line)) {
            lines.push_back(line);
        }
        file.close();

        // 查找old_string出现的位置
        std::vector<size_t> occurrences = findStringOccurrences(lines, oldString);

        if (occurrences.empty()) {
            error = "未找到要替换的内容";
            return false;
        }

        // 执行替换
        replaceCount = 0;
        for (size_t pos : occurrences) {
            size_t lineIndex = pos;
            size_t charIndex = 0;

            // 找到具体是哪一行
            size_t currentPos = 0;
            for (size_t i = 0; i < lines.size(); ++i) {
                size_t lineLen = lines[i].length() + 1;  // +1 for newline
                if (currentPos + lineLen > pos) {
                    lineIndex = i;
                    charIndex = pos - currentPos;
                    break;
                }
                currentPos += lineLen;
            }

            // 在该行中替换
            if (lineIndex < lines.size()) {
                size_t findPos = lines[lineIndex].find(oldString, charIndex);
                if (findPos != std::string::npos) {
                    lines[lineIndex].replace(findPos, oldString.length(), newString);
                    replaceCount++;
                    affectedLines.push_back(static_cast<int>(lineIndex) + 1);  // 1-based line number
                }
            }
        }

        // 写回文件
        std::ofstream outFile(path);
        if (!outFile.is_open()) {
            error = "无法写入文件: " + path;
            return false;
        }

        for (const auto& l : lines) {
            outFile << l << "\n";
        }
        outFile.close();

        return true;

    } catch (const std::exception& e) {
        error = std::string("编辑文件失败: ") + e.what();
        return false;
    }
}

std::vector<size_t> EditTool::findStringOccurrences(const std::vector<std::string>& lines,
                                                     const std::string& searchString) {
    std::vector<size_t> occurrences;
    size_t currentPos = 0;

    for (const auto& line : lines) {
        size_t pos = 0;
        while ((pos = line.find(searchString, pos)) != std::string::npos) {
            occurrences.push_back(currentPos + pos);
            pos += searchString.length();
        }
        currentPos += line.length() + 1;  // +1 for newline
    }

    return occurrences;
}

int EditTool::getLineNumber(const std::vector<std::string>& lines, size_t position) {
    size_t currentPos = 0;
    for (size_t i = 0; i < lines.size(); ++i) {
        size_t lineLen = lines[i].length() + 1;  // +1 for newline
        if (currentPos + lineLen > position) {
            return static_cast<int>(i) + 1;  // 1-based line number
        }
        currentPos += lineLen;
    }
    return -1;
}

bool EditTool::validateOldStringExists(const std::string& path, const std::string& oldString) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();

    return content.find(oldString) != std::string::npos;
}

} // namespace roboclaw
