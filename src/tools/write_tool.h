// Write工具 - 创建新文件或覆盖现有文件

#ifndef ROBOCLAW_TOOLS_WRITE_TOOL_H
#define ROBOCLAW_TOOLS_WRITE_TOOL_H

#include "tool_base.h"
#include <fstream>
#include <filesystem>

namespace roboclaw {

class WriteTool : public ToolBase {
public:
    WriteTool();
    ~WriteTool() override = default;

    // 获取工具描述
    ToolDescription getToolDescription() const override;

    // 验证参数
    bool validateParams(const json& params) const override;

    // 执行工具
    ToolResult execute(const json& params) override;

private:
    // 写入文件（使用临时文件+重命名实现原子写入）
    bool writeFile(const std::string& path, const std::string& content, std::string& error);

    // 创建目录（如果不存在）
    bool ensureDirectoryExists(const std::string& path);

    // 检查路径是否有效
    bool isValidPath(const std::string& path) const;
};

} // namespace roboclaw

#endif // ROBOCLAW_TOOLS_WRITE_TOOL_H
