// Edit工具 - 精确替换文件内容

#ifndef ROBOCLAW_TOOLS_EDIT_TOOL_H
#define ROBOCLAW_TOOLS_EDIT_TOOL_H

#include "tool_base.h"
#include <fstream>
#include <filesystem>
#include <vector>

namespace roboclaw {

class EditTool : public ToolBase {
public:
    EditTool();
    ~EditTool() override = default;

    // 获取工具描述
    ToolDescription getDescription() const override;

    // 验证参数
    bool validateParams(const json& params) const override;

    // 执行工具
    ToolResult execute(const json& params) override;

private:
    // 编辑文件（替换内容）
    bool editFile(const std::string& path, const std::string& oldString,
                  const std::string& newString, std::string& error,
                  int& replaceCount, std::vector<int>& affectedLines);

    // 查找字符串在文件中的位置
    std::vector<size_t> findStringOccurrences(const std::vector<std::string>& lines,
                                              const std::string& searchString);

    // 获取行号
    int getLineNumber(const std::vector<std::string>& lines, size_t position);

    // 验证old_string存在
    bool validateOldStringExists(const std::string& path, const std::string& oldString);
};

} // namespace roboclaw

#endif // ROBOCLAW_TOOLS_EDIT_TOOL_H
