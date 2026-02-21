// Read工具 - 读取文件内容

#ifndef ROBOCLAW_TOOLS_READ_TOOL_H
#define ROBOCLAW_TOOLS_READ_TOOL_H

#include "tool_base.h"
#include <fstream>
#include <filesystem>
#include <vector>

namespace roboclaw {

class ReadTool : public ToolBase {
public:
    ReadTool();
    ~ReadTool() override = default;

    // 获取工具描述
    ToolDescription getToolDescription() const override;

    // 验证参数
    bool validateParams(const json& params) const override;

    // 执行工具
    ToolResult execute(const json& params) override;

private:
    // 读取文件内容
    std::string readFile(const std::string& path, int offset, int limit,
                        int& totalLines, std::string& error);

    // 检测文件编码
    std::string detectEncoding(const std::string& path);

    // 读取文件的指定行
    std::vector<std::string> readLines(const std::string& path, int offset, int limit);

    // 检查文件大小
    bool checkFileSize(const std::string& path, size_t maxSizeMB);
};

} // namespace roboclaw

#endif // ROBOCLAW_TOOLS_READ_TOOL_H
