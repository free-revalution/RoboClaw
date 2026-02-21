// 配置管理器单元测试 / Config Manager Unit Tests

#include <gtest/gtest.h>
#include "../../src/storage/config_manager.h"
#include <filesystem>
#include <fstream>

using namespace roboclaw;

class ConfigManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建临时测试配置目录
        test_config_dir_ = "/tmp/roboclaw_test_" + std::to_string(std::random_device{}());
        std::filesystem::create_directories(test_config_dir_);
        test_config_path_ = test_config_dir_ + "/config.toml";
    }

    void TearDown() override {
        // 清理临时目录
        if (std::filesystem::exists(test_config_dir_)) {
            std::filesystem::remove_all(test_config_dir_);
        }
    }

    std::string test_config_dir_;
    std::string test_config_path_;
};

// 测试语言转换函数 / Test language conversion
TEST_F(ConfigManagerTest, LanguageConversion) {
    EXPECT_EQ(ConfigManager::languageToString(Language::CHINESE), "chinese");
    EXPECT_EQ(ConfigManager::languageToString(Language::ENGLISH), "english");

    EXPECT_EQ(ConfigManager::stringToLanguage("chinese"), Language::CHINESE);
    EXPECT_EQ(ConfigManager::stringToLanguage("english"), Language::ENGLISH);
    EXPECT_EQ(ConfigManager::stringToLanguage("zh"), Language::CHINESE);
    EXPECT_EQ(ConfigManager::stringToLanguage("en"), Language::ENGLISH);
    EXPECT_EQ(ConfigManager::stringToLanguage("invalid"), Language::CHINESE);  // 默认
}

// 测试提供商转换 / Test provider conversion
TEST_F(ConfigManagerTest, ProviderConversion) {
    EXPECT_EQ(ConfigManager::providerToString(ProviderType::ANTHROPIC), "anthropic");
    EXPECT_EQ(ConfigManager::providerToString(ProviderType::OPENAI), "openai");
    EXPECT_EQ(ConfigManager::providerToString(ProviderType::GEMINI), "gemini");

    EXPECT_EQ(ConfigManager::stringToProvider("anthropic"), ProviderType::ANTHROPIC);
    EXPECT_EQ(ConfigManager::stringToProvider("openai"), ProviderType::OPENAI);
}

// 测试默认配置初始化 / Test default config initialization
TEST_F(ConfigManagerTest, DefaultInitialization) {
    ConfigManager configMgr;
    configMgr.initializeDefaults();

    const auto& config = configMgr.getConfig();

    // 检查语言默认值 / Check language default
    EXPECT_EQ(config.language, Language::CHINESE);

    // 检查提供商默认值 / Check provider default
    EXPECT_EQ(config.default_config.provider, ProviderType::ANTHROPIC);
    EXPECT_EQ(config.default_config.model, "claude-sonnet-4-20250514");

    // 检查行为配置 / Check behavior config
    EXPECT_EQ(config.behavior.max_retries, 3);
    EXPECT_EQ(config.behavior.timeout, 60);
    EXPECT_TRUE(config.behavior.verbose);

    // 检查工具配置 / Check tools config
    EXPECT_EQ(config.tools.bash_timeout, 30);
    EXPECT_FALSE(config.tools.forbidden_commands.empty());
}

// 测试配置保存和加载 / Test config save and load
TEST_F(ConfigManagerTest, SaveAndLoad) {
    ConfigManager configMgr;
    configMgr.initializeDefaults();

    // 修改配置 / Modify config
    configMgr.setLanguage(Language::ENGLISH);
    configMgr.setProvider(ProviderType::OPENAI);
    configMgr.setModel("gpt-4o");
    configMgr.setApiKey(ProviderType::OPENAI, "test-api-key-12345");

    // 保存配置 / Save config
    bool saved = configMgr.save(test_config_path_);
    ASSERT_TRUE(saved);
    EXPECT_TRUE(std::filesystem::exists(test_config_path_));

    // 加载配置 / Load config
    ConfigManager configMgr2;
    bool loaded = configMgr2.load(test_config_path_);
    ASSERT_TRUE(loaded);

    const auto& config = configMgr2.getConfig();
    EXPECT_EQ(config.language, Language::ENGLISH);
    EXPECT_EQ(config.default_config.provider, ProviderType::OPENAI);
    EXPECT_EQ(config.default_config.model, "gpt-4o");
    EXPECT_EQ(config.providers.at(ProviderType::OPENAI).api_key, "test-api-key-12345");
}

// 测试配置验证 / Test config validation
TEST_F(ConfigManagerTest, ConfigValidation) {
    ConfigManager configMgr;
    configMgr.initializeDefaults();

    // 没有API key的配置应该无效 / Config without API key should be invalid
    EXPECT_FALSE(configMgr.getConfig().validate());

    // 添加API key后应该有效 / Should be valid after adding API key
    configMgr.setApiKey(ProviderType::ANTHROPIC, "sk-ant-test-key-12345");
    EXPECT_TRUE(configMgr.getConfig().validate());
}

// 测试TOML生成和解析 / Test TOML generation and parsing
TEST_F(ConfigManagerTest, TomlGeneration) {
    ConfigManager configMgr;
    configMgr.initializeDefaults();
    configMgr.setLanguage(Language::ENGLISH);

    std::string toml = configMgr.generateToml();

    // 检查TOML内容 / Check TOML content
    EXPECT_NE(toml.find("language = \"english\""), std::string::npos);
    EXPECT_NE(toml.find("provider = \"anthropic\""), std::string::npos);
    EXPECT_NE(toml.find("model = \"claude-sonnet-4-20250514\""), std::string::npos);
}

// 运行所有测试 / Run all tests
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
