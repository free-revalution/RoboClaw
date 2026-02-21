// 工具单元测试 / Tools Unit Tests

#include <gtest/gtest.h>
#include "../../src/tools/tool_base.h"
#include "../../src/tools/read_tool.h"
#include "../../src/tools/write_tool.h"
#include "../../src/tools/bash_tool.h"
#include "../../src/tools/serial_tool.h"
#include <filesystem>
#include <fstream>

using namespace roboclaw;

class ToolsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建临时测试目录
        test_dir_ = "/tmp/roboclaw_tools_test_" + std::to_string(std::random_device{}());
        std::filesystem::create_directories(test_dir_);
    }

    void TearDown() override {
        // 清理临时目录
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }

    std::string test_dir_;
};

// 测试Read工具 / Test Read tool
TEST_F(ToolsTest, ReadTool) {
    ReadTool readTool;

    // 创建测试文件 / Create test file
    std::string test_file = test_dir_ + "/test_read.txt";
    std::ofstream file(test_file);
    file << "Hello, RoboClaw!\nThis is a test file.";
    file.close();

    // 读取文件 / Read file
    json params;
    params["file"] = test_file;

    ToolResult result = readTool.execute(params);

    EXPECT_TRUE(result.success);
    EXPECT_NE(result.content.find("Hello, RoboClaw!"), std::string::npos);
}

// 测试Write工具 / Test Write tool
TEST_F(ToolsTest, WriteTool) {
    WriteTool writeTool;

    std::string test_file = test_dir_ + "/test_write.txt";

    // 写入文件 / Write file
    json params;
    params["file"] = test_file;
    params["content"] = "Test content for WriteTool";

    ToolResult result = writeTool.execute(params);

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(std::filesystem::exists(test_file));

    // 验证内容 / Verify content
    std::ifstream file(test_file);
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    EXPECT_EQ(content, "Test content for WriteTool");
}

// 测试Edit工具 / Test Edit tool
TEST_F(ToolsTest, EditTool) {
    // 先创建文件 / First create file
    std::string test_file = test_dir_ + "/test_edit.txt";
    std::ofstream file(test_file);
    file << "Hello World\nFoo Bar Baz";
    file.close();

    EditTool editTool;

    // 替换内容 / Replace content
    json params;
    params["file"] = test_file;
    params["old_str"] = "World";
    params["new_str"] = "RoboClaw";

    ToolResult result = editTool.execute(params);

    EXPECT_TRUE(result.success);

    // 验证替换 / Verify replacement
    std::ifstream edited_file(test_file);
    std::string content((std::istreambuf_iterator<char>(edited_file)),
                        std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("Hello RoboClaw"), std::string::npos);
    EXPECT_EQ(content.find("Hello World"), std::string::npos);
}

// 测试Bash工具 / Test Bash tool
TEST_F(ToolsTest, BashTool) {
    BashTool bashTool;
    bashTool.setTimeout(5);  // 5秒超时 / 5 second timeout

    json params;
    params["command"] = "echo 'Hello from BashTool'";

    ToolResult result = bashTool.execute(params);

    EXPECT_TRUE(result.success);
    EXPECT_NE(result.content.find("Hello from BashTool"), std::string::npos);
}

// 测试Bash工具禁止的命令 / Test Bash tool forbidden commands
TEST_F(ToolsTest, BashToolForbiddenCommands) {
    BashTool bashTool;
    bashTool.setForbiddenCommands({"rm -rf"});

    json params;
    params["command"] = "rm -rf /";

    ToolResult result = bashTool.execute(params);

    EXPECT_FALSE(result.success);
    EXPECT_NE(result.error_message.find("forbidden"), std::string::npos);
}

// 测试工具描述 / Test tool descriptions
TEST_F(ToolsTest, ToolDescriptions) {
    ReadTool readTool;
    WriteTool writeTool;
    EditTool editTool;
    BashTool bashTool;

    // Read工具描述 / Read tool description
    auto readDesc = readTool.getToolDescription();
    EXPECT_EQ(readDesc.name, "read");
    EXPECT_FALSE(readDesc.description.empty());

    // Write工具描述 / Write tool description
    auto writeDesc = writeTool.getToolDescription();
    EXPECT_EQ(writeDesc.name, "write");

    // Edit工具描述 / Edit tool description
    auto editDesc = editTool.getToolDescription();
    EXPECT_EQ(editDesc.name, "edit");

    // Bash工具描述 / Bash tool description
    auto bashDesc = bashTool.getToolDescription();
    EXPECT_EQ(bashDesc.name, "bash");
}

// 测试工具参数验证 / Test tool parameter validation
TEST_F(ToolsTest, ToolParameterValidation) {
    ReadTool readTool;

    // 缺少必需参数 / Missing required parameter
    json params;  // 空 / empty
    EXPECT_FALSE(readTool.validateParams(params));

    // 添加必需参数 / Add required parameter
    params["file"] = "/tmp/test.txt";
    EXPECT_TRUE(readTool.validateParams(params));
}

// 测试工具注册表 / Test tool registry
TEST_F(ToolsTest, ToolRegistry) {
    auto& registry = ToolRegistry::getInstance();

    // 注册工具 / Register tool
    auto readTool = std::make_shared<ReadTool>();
    registry.registerTool("test_read", readTool);

    // 检查工具是否存在 / Check if tool exists
    EXPECT_TRUE(registry.hasTool("test_read"));

    // 获取工具 / Get tool
    auto retrievedTool = registry.getTool("test_read");
    EXPECT_NE(retrievedTool, nullptr);
    EXPECT_EQ(retrievedTool->getName(), "test_read");

    // 获取所有工具描述 / Get all tool descriptions
    auto descriptions = registry.getAllToolDescriptions();
    EXPECT_GT(descriptions.size(), 0);
}

// 测试错误结果 / Test error results
TEST_F(ToolsTest, ErrorResults) {
    ReadTool readTool;

    // 尝试读取不存在的文件 / Try to read non-existent file
    json params;
    params["file"] = "/nonexistent/file/path.txt";

    ToolResult result = readTool.execute(params);

    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

// 测试元数据 / Test metadata
TEST_F(ToolsTest, ToolMetadata) {
    WriteTool writeTool;
    std::string test_file = test_dir_ + "/metadata_test.txt";

    json params;
    params["file"] = test_file;
    params["content"] = "Test content";

    ToolResult result = writeTool.execute(params);

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.metadata.is_object());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
