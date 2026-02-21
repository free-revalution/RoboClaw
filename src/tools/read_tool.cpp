// ReadTool实现

#include "read_tool.h"
#include <sstream>
#include <algorithm>

namespace roboclaw {

ReadTool::ReadTool()
    : ToolBase("read", "读取文件内容") {
}

ToolDescription ReadTool::getToolDescription() const {
    ToolDescription desc;
    desc.name = name_;
    desc.description = description_;

    desc.parameters = {
        {"path", "string", "文件路径（必需）", true, ""},
        {"offset", "integer", "起始行号（可选，默认0）", false, "0"},
        {"limit", "integer", "读取行数（可选，默认全部）", false, "0"}
    };

    return desc;
}

bool ReadTool::validateParams(const json& params) const {
    if (!hasRequiredParam(params, "path")) {
        return false;
    }

    std::string path = getStringParam(params, "path");
    if (path.empty()) {
        return false;
    }

    int offset = getIntParam(params, "offset", 0);
    if (offset < 0) {
        return false;
    }

    int limit = getIntParam(params, "limit", 0);
    if (limit < 0) {
        return false;
    }

    return true;
}

ToolResult ReadTool::execute(const json& params) {
    if (!validateParams(params)) {
        return ToolResult::error("参数验证失败：path是必需参数");
    }

    std::string path = getStringParam(params, "path");
    int offset = getIntParam(params, "offset", 0);
    int limit = getIntParam(params, "limit", 0);

    LOG_DEBUG("读取文件: " + path + " (offset=" + std::to_string(offset) + ", limit=" + std::to_string(limit) + ")");

    // 检查文件是否存在
    if (!std::filesystem::exists(path)) {
        return ToolResult::error("文件不存在: " + path);
    }

    // 检查是否是文件
    if (!std::filesystem::is_regular_file(path)) {
        return ToolResult::error("路径不是常规文件: " + path);
    }

    // 检查文件大小（10MB限制）
    const size_t MAX_SIZE_MB = 10;
    if (!checkFileSize(path, MAX_SIZE_MB)) {
        return ToolResult::error("文件过大，超过" + std::to_string(MAX_SIZE_MB) + "MB限制");
    }

    // 读取文件
    int totalLines = 0;
    std::string error;
    std::string content = readFile(path, offset, limit, totalLines, error);

    if (!error.empty()) {
        return ToolResult::error(error);
    }

    // 构建元数据
    json metadata;
    metadata["path"] = path;
    metadata["total_lines"] = totalLines;
    int lines_read = limit > 0 ? std::min(limit, totalLines - offset) : totalLines - offset;
    metadata["lines_read"] = lines_read;
    metadata["offset"] = offset;
    metadata["encoding"] = detectEncoding(path);
    metadata["file_size"] = std::filesystem::file_size(path);

    LOG_DEBUG("文件读取成功: " + std::to_string(lines_read) + " 行");

    return ToolResult::ok(content, metadata);
}

std::string ReadTool::readFile(const std::string& path, int offset, int limit,
                                int& totalLines, std::string& error) {
    std::vector<std::string> lines = readLines(path, offset, limit);
    totalLines = lines.size();

    if (lines.empty()) {
        return "";
    }

    // 如果指定了offset和limit，实际读取的行数可能不同
    if (offset >= static_cast<int>(lines.size())) {
        error = "offset超出文件行数";
        return "";
    }

    // 构建内容
    std::stringstream ss;
    int startLine = offset;
    int endLine = limit > 0 ? std::min(offset + limit, static_cast<int>(lines.size()))
                            : static_cast<int>(lines.size());

    for (int i = startLine; i < endLine; ++i) {
        ss << lines[i] << "\n";
    }

    return ss.str();
}

std::vector<std::string> ReadTool::readLines(const std::string& path, int offset, int limit) {
    std::vector<std::string> lines;
    std::ifstream file(path);

    if (!file.is_open()) {
        return lines;
    }

    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }

    return lines;
}

std::string ReadTool::detectEncoding(const std::string& path) {
    // 简化实现：默认返回UTF-8
    // 实际可以通过BOM检测或字节分析来确定编码
    return "UTF-8";
}

bool ReadTool::checkFileSize(const std::string& path, size_t maxSizeMB) {
    try {
        size_t fileSize = std::filesystem::file_size(path);
        size_t maxSizeBytes = maxSizeMB * 1024 * 1024;
        return fileSize <= maxSizeBytes;
    } catch (...) {
        return false;
    }
}

} // namespace roboclaw
