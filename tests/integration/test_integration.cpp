// 集成测试 / Integration Tests

#include <gtest/gtest.h>
#include "../../src/storage/config_manager.h"
#include "../../src/agent/tool_executor.h"
#include "../../src/tools/tool_base.h"
#include <filesystem>
#include <fstream>

using namespace roboclaw;

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建临时测试目录
        test_dir_ = "/tmp/roboclaw_integration_test_" + std::to_string(std::random_device{}());
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

// 测试配置管理器和工具执行器集成 / Test config manager and tool executor integration
TEST_F(IntegrationTest, ConfigManagerAndToolExecutor) {
    ConfigManager configMgr;
    configMgr.initializeDefaults();

    // 设置配置 / Setup configuration
    configMgr.setLanguage(Language::ENGLISH);
    configMgr.setProvider(ProviderType::ANTHROPIC);
    configMgr.setModel("claude-sonnet-4-20250514");
    configMgr.setApiKey(ProviderType::ANTHROPIC, "test-api-key");

    // 验证配置 / Verify configuration
    EXPECT_EQ(configMgr.getLanguage(), Language::ENGLISH);
    EXPECT_EQ(configMgr.getProvider(), ProviderType::ANTHROPIC);
    EXPECT_TRUE(configMgr.getConfig().validate());
}

// 测试多工具工作流 / Test multi-tool workflow
TEST_F(IntegrationTest, MultiToolWorkflow) {
    ToolExecutor executor;
    executor.initialize();

    // 1. 创建文件 / 1. Create file
    std::string test_file = test_dir_ + "/workflow_test.txt";
    json write_params;
    write_params["file"] = test_file;
    write_params["content"] = "Original content";

    ToolResult write_result = executor.execute("write", write_params);
    EXPECT_TRUE(write_result.success);

    // 2. 读取文件 / 2. Read file
    json read_params;
    read_params["file"] = test_file;

    ToolResult read_result = executor.execute("read", read_params);
    EXPECT_TRUE(read_result.success);
    EXPECT_NE(read_result.content.find("Original content"), std::string::npos);

    // 3. 编辑文件 / 3. Edit file
    json edit_params;
    edit_params["file"] = test_file;
    edit_params["old_str"] = "Original";
    edit_params["new_str"] = "Modified";

    ToolResult edit_result = executor.execute("edit", edit_params);
    EXPECT_TRUE(edit_result.success);

    // 4. 再次读取验证编辑 / 4. Read again to verify edit
    ToolResult read_result2 = executor.execute("read", read_params);
    EXPECT_TRUE(read_result2.success);
    EXPECT_NE(read_result2.content.find("Modified content"), std::string::npos);
    EXPECT_EQ(read_result2.content.find("Original content"), std::string::npos);
}

// 测试工具执行器并发执行 / Test tool executor concurrent execution
TEST_F(IntegrationTest, ConcurrentToolExecution) {
    ToolExecutor executor;
    executor.initialize();

    // 创建多个测试文件 / Create multiple test files
    std::vector<std::string> test_files;
    for (int i = 0; i < 5; ++i) {
        std::string file = test_dir_ + "/concurrent_" + std::to_string(i) + ".txt";
        test_files.push_back(file);

        json write_params;
        write_params["file"] = file;
        write_params["content"] = "Content " + std::to_string(i);

        ToolResult result = executor.execute("write", write_params);
        EXPECT_TRUE(result.success);
    }

    // 验证所有文件都已创建 / Verify all files were created
    for (const auto& file : test_files) {
        EXPECT_TRUE(std::filesystem::exists(file));
    }
}

// 测试Bash工具与文件操作集成 / Test Bash tool integration with file operations
TEST_F(IntegrationTest, BashToolFileIntegration) {
    ToolExecutor executor;
    executor.initialize();

    // 使用Bash创建文件 / Use Bash to create file
    std::string test_file = test_dir_ + "/bash_created.txt";

    json bash_params;
    bash_params["command"] = "echo 'Bash created this' > " + test_file;

    ToolResult bash_result = executor.execute("bash", bash_params);
    EXPECT_TRUE(bash_result.success);

    // 使用Read工具读取 / Use Read tool to read
    json read_params;
    read_params["file"] = test_file;

    ToolResult read_result = executor.execute("read", read_params);
    EXPECT_TRUE(read_result.success);
    EXPECT_NE(read_result.content.find("Bash created this"), std::string::npos);
}

// 测试配置持久化 / Test configuration persistence
TEST_F(IntegrationTest, ConfigurationPersistence) {
    std::string config_path = test_dir_ + "/config.toml";

    // 保存配置 / Save configuration
    {
        ConfigManager configMgr;
        configMgr.initializeDefaults();
        configMgr.setLanguage(Language::ENGLISH);
        configMgr.setProvider(ProviderType::OPENAI);
        configMgr.setModel("gpt-4o");
        configMgr.setApiKey(ProviderType::OPENAI, "sk-test-key");

        ASSERT_TRUE(configMgr.save(config_path));
    }

    // 加载配置 / Load configuration
    {
        ConfigManager configMgr2;
        ASSERT_TRUE(configMgr2.load(config_path));

        const auto& config = configMgr2.getConfig();
        EXPECT_EQ(config.language, Language::ENGLISH);
        EXPECT_EQ(config.default_config.provider, ProviderType::OPENAI);
        EXPECT_EQ(config.default_config.model, "gpt-4o");
        EXPECT_EQ(config.providers.at(ProviderType::OPENAI).api_key, "sk-test-key");
    }
}

// 测试语言切换集成 / Test language switching integration
TEST_F(IntegrationTest, LanguageSwitchingIntegration) {
    ConfigManager configMgr;
    configMgr.initializeDefaults();

    // 测试中文配置 / Test Chinese configuration
    configMgr.setLanguage(Language::CHINESE);
    configMgr.setProvider(ProviderType::ANTHROPIC);
    EXPECT_EQ(configMgr.getLanguage(), Language::CHINESE);

    // 保存并重新加载 / Save and reload
    std::string config_path = test_dir_ + "/config_lang.toml";
    ASSERT_TRUE(configMgr.save(config_path));

    ConfigManager configMgr2;
    ASSERT_TRUE(configMgr2.load(config_path));
    EXPECT_EQ(configMgr2.getLanguage(), Language::CHINESE);

    // 切换到英文 / Switch to English
    configMgr2.setLanguage(Language::ENGLISH);
    EXPECT_EQ(configMgr2.getLanguage(), Language::ENGLISH);
}

// 测试工具描述获取 / Test tool description retrieval
TEST_F(IntegrationTest, ToolDescriptionsRetrieval) {
    ToolExecutor executor;
    executor.initialize();

    auto descriptions = executor.getAllToolDescriptions();

    // 应该至少有7个工具：read, write, edit, bash, serial, browser, agent
    EXPECT_GE(descriptions.size(), 7);

    // 检查每个工具都有必要的字段 / Check each tool has necessary fields
    for (const auto& desc : descriptions) {
        EXPECT_FALSE(desc.name.empty());
        EXPECT_FALSE(desc.description.empty());
    }
}

// 测试错误处理 / Test error handling
TEST_F(IntegrationTest, ErrorHandling) {
    ToolExecutor executor;
    executor.initialize();

    // 尝试读取不存在的文件 / Try to read non-existent file
    json read_params;
    read_params["file"] = "/nonexistent/file.txt";

    ToolResult result = executor.execute("read", read_params);
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

// 测试工具注册表完整性 / Test tool registry integrity
TEST_F(IntegrationTest, ToolRegistryIntegrity) {
    ToolExecutor executor;
    executor.initialize();

    // 检查所有预期的工具都已注册 / Check all expected tools are registered
    EXPECT_TRUE(executor.hasTool("read"));
    EXPECT_TRUE(executor.hasTool("write"));
    EXPECT_TRUE(executor.hasTool("edit"));
    EXPECT_TRUE(executor.hasTool("bash"));
    EXPECT_TRUE(executor.hasTool("serial"));
    EXPECT_TRUE(executor.hasTool("browser"));
    EXPECT_TRUE(executor.hasTool("agent"));

    // 检查不存在的工具 / Check non-existent tool
    EXPECT_FALSE(executor.hasTool("nonexistent_tool"));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
