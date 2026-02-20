// WriteTool实现

#include "write_tool.h"
#include <sstream>
#include <algorithm>

namespace roboclaw {

WriteTool::WriteTool()
    : ToolBase("write", "创建新文件或覆盖现有文件") {
}

ToolDescription WriteTool::getDescription() const {
    ToolDescription desc;
    desc.name = name_;
    desc.description = description_;

    desc.parameters = {
        {"path", "string", "文件路径（必需）", true, ""},
        {"content", "string", "文件内容（必需）", true, ""}
    };

    return desc;
}

bool WriteTool::validateParams(const json& params) const {
    if (!hasRequiredParam(params, "path")) {
        return false;
    }
    if (!hasRequiredParam(params, "content")) {
        return false;
    }

    std::string path = getStringParam(params, "path");
    if (path.empty()) {
        return false;
    }

    return isValidPath(path);
}

ToolResult WriteTool::execute(const json& params) {
    if (!validateParams(params)) {
        return ToolResult::error("参数验证失败：path和content都是必需参数");
    }

    std::string path = getStringParam(params, "path");
    std::string content = getStringParam(params, "content");

    LOG_DEBUG("写入文件: " + path + " (" + std::to_string(content.length()) + " 字节)");

    // 确保目录存在
    if (!ensureDirectoryExists(path)) {
        return ToolResult::error("无法创建目录: " + path);
    }

    // 写入文件
    std::string error;
    if (!writeFile(path, content, error)) {
        return ToolResult::error(error);
    }

    // 构建元数据
    json metadata;
    metadata["path"] = path;
    metadata["bytes_written"] = content.length();
    metadata["overwrite"] = std::filesystem::exists(path);

    LOG_DEBUG("文件写入成功: " + path);

    return ToolResult::ok("文件已成功写入: " + path, metadata);
}

bool WriteTool::writeFile(const std::string& path, const std::string& content, std::string& error) {
    // 使用原子写入策略：先写临时文件，成功后重命名
    std::filesystem::path filePath(path);
    std::filesystem::path tempPath = filePath.parent_path() / (filePath.filename().string() + ".tmp");

    try {
        // 写入临时文件
        std::ofstream tempFile(tempPath, std::ios::binary);
        if (!tempFile.is_open()) {
            error = "无法创建临时文件: " + tempPath.string();
            return false;
        }

        tempFile << content;
        tempFile.close();

        // 检查是否成功写入
        if (tempFile.fail()) {
            error = "写入临时文件失败";
            std::filesystem::remove(tempPath);
            return false;
        }

        // 如果目标文件已存在，先备份
        std::filesystem::path backupPath;
        if (std::filesystem::exists(filePath)) {
            backupPath = filePath.parent_path() / (filePath.filename().string() + ".bak");
            std::filesystem::copy_file(filePath, backupPath,
                std::filesystem::copy_options::overwrite_existing);
        }

        // 重命名临时文件为目标文件
        std::filesystem::rename(tempPath, filePath);

        // 成功后删除备份
        if (!backupPath.empty() && std::filesystem::exists(backupPath)) {
            std::filesystem::remove(backupPath);
        }

        return true;

    } catch (const std::exception& e) {
        error = std::string("写入文件失败: ") + e.what();

        // 清理临时文件
        if (std::filesystem::exists(tempPath)) {
            std::filesystem::remove(tempPath);
        }

        return false;
    }
}

bool WriteTool::ensureDirectoryExists(const std::string& path) {
    try {
        std::filesystem::path filePath(path);
        if (filePath.has_parent_path()) {
            std::filesystem::create_directories(filePath.parent_path());
        }
        return true;
    } catch (...) {
        return false;
    }
}

bool WriteTool::isValidPath(const std::string& path) {
    try {
        std::filesystem::path filePath(path);

        // 检查路径是否为空
        if (path.empty()) {
            return false;
        }

        // 检查路径是否包含非法字符（Windows）
        #ifdef PLATFORM_WINDOWS
        std::string invalidChars = "<>:\"|?*";
        for (char c : invalidChars) {
            if (path.find(c) != std::string::npos) {
                return false;
            }
        }
        #endif

        // 检查是否是绝对路径或相对路径
        return true;

    } catch (...) {
        return false;
    }
}

} // namespace roboclaw
