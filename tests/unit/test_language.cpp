// 语言支持单元测试 / Language Support Unit Tests

#include <gtest/gtest.h>
#include "../../src/storage/config_manager.h"
#include <map>

using namespace roboclaw;

// 测试语言枚举值 / Test language enum values
TEST(LanguageTest, LanguageEnumValues) {
    // 测试Language枚举的正确性 / Test Language enum correctness
    EXPECT_EQ(static_cast<int>(Language::CHINESE), 0);
    EXPECT_EQ(static_cast<int>(Language::ENGLISH), 1);
}

// 测试语言字符串转换 / Test language string conversion
TEST(LanguageTest, LanguageStringConversion) {
    // 测试所有语言到字符串的转换 / Test all language to string conversions
    EXPECT_EQ(ConfigManager::languageToString(Language::CHINESE), "chinese");
    EXPECT_EQ(ConfigManager::languageToString(Language::ENGLISH), "english");

    // 测试字符串到语言的转换 / Test string to language conversions
    EXPECT_EQ(ConfigManager::stringToLanguage("chinese"), Language::CHINESE);
    EXPECT_EQ(ConfigManager::stringToLanguage("english"), Language::ENGLISH);

    // 测试大小写不敏感 / Test case insensitivity
    EXPECT_EQ(ConfigManager::stringToLanguage("CHINESE"), Language::CHINESE);
    EXPECT_EQ(ConfigManager::stringToLanguage("English"), Language::ENGLISH);

    // 测试别名 / Test aliases
    EXPECT_EQ(ConfigManager::stringToLanguage("zh"), Language::CHINESE);
    EXPECT_EQ(ConfigManager::stringToLanguage("zh-cn"), Language::CHINESE);
    EXPECT_EQ(ConfigManager::stringToLanguage("en"), Language::ENGLISH);

    // 测试无效值默认返回中文 / Test invalid value defaults to Chinese
    EXPECT_EQ(ConfigManager::stringToLanguage("invalid"), Language::CHINESE);
    EXPECT_EQ(ConfigManager::stringToLanguage("fr"), Language::CHINESE);
}

// 测试配置中的语言设置 / Test language setting in config
TEST(LanguageTest, LanguageInConfig) {
    Config config;

    // 测试默认语言 / Test default language
    EXPECT_EQ(config.language, Language::CHINESE);

    // 测试设置语言 / Test setting language
    config.language = Language::ENGLISH;
    EXPECT_EQ(config.language, Language::ENGLISH);

    config.language = Language::CHINESE;
    EXPECT_EQ(config.language, Language::CHINESE);
}

// 测试语言配置持久化 / Test language configuration persistence
TEST(LanguageTest, LanguagePersistence) {
    ConfigManager configMgr;
    configMgr.initializeDefaults();

    // 设置语言 / Set language
    configMgr.setLanguage(Language::ENGLISH);
    EXPECT_EQ(configMgr.getLanguage(), Language::ENGLISH);

    // 修改为中文 / Change to Chinese
    configMgr.setLanguage(Language::CHINESE);
    EXPECT_EQ(configMgr.getLanguage(), Language::CHINESE);
}

// 测试TOML中的语言配置 / Test language configuration in TOML
TEST(LanguageTest, LanguageInToml) {
    ConfigManager configMgr;
    configMgr.initializeDefaults();

    // 生成TOML / Generate TOML
    std::string toml = configMgr.generateToml();

    // 验证语言字段存在 / Verify language field exists
    EXPECT_NE(toml.find("language ="), std::string::npos);

    // 测试英文配置 / Test English configuration
    configMgr.setLanguage(Language::ENGLISH);
    toml = configMgr.generateToml();
    EXPECT_NE(toml.find("language = \"english\""), std::string::npos);

    // 测试中文配置 / Test Chinese configuration
    configMgr.setLanguage(Language::CHINESE);
    toml = configMgr.generateToml();
    EXPECT_NE(toml.find("language = \"chinese\""), std::string::npos);
}

// 测试TOML解析语言配置 / Test TOML parsing language configuration
TEST(LanguageTest, ParseLanguageFromToml) {
    std::string testToml = R"(
[default]
provider = "anthropic"
model = "claude-sonnet-4-20250514"
language = "english"

[providers.anthropic]
api_key = "test-key"
base_url = "https://api.anthropic.com"
models = ["claude-sonnet-4-20250514"]
)";

    ConfigManager configMgr;
    bool parsed = configMgr.parseToml(testToml);

    EXPECT_TRUE(parsed);
    EXPECT_EQ(configMgr.getLanguage(), Language::ENGLISH);
}

// 测试所有支持的语言 / Test all supported languages
TEST(LanguageTest, AllSupportedLanguages) {
    std::vector<Language> allLanguages = {
        Language::CHINESE,
        Language::ENGLISH
    };

    for (auto lang : allLanguages) {
        // 检查每种语言都可以转换 / Check each language can be converted
        std::string langStr = ConfigManager::languageToString(lang);
        Language convertedLang = ConfigManager::stringToLanguage(langStr);
        EXPECT_EQ(lang, convertedLang);
    }
}

// 测试语言配置与提供商配置的独立性 / Test independence of language and provider config
TEST(LanguageTest, LanguageProviderIndependence) {
    ConfigManager configMgr;
    configMgr.initializeDefaults();

    // 设置不同的提供商和语言 / Set different provider and language
    configMgr.setProvider(ProviderType::OPENAI);
    configMgr.setLanguage(Language::ENGLISH);

    EXPECT_EQ(configMgr.getProvider(), ProviderType::OPENAI);
    EXPECT_EQ(configMgr.getLanguage(), Language::ENGLISH);

    // 更改提供商不应影响语言 / Changing provider should not affect language
    configMgr.setProvider(ProviderType::ANTHROPIC);
    EXPECT_EQ(configMgr.getLanguage(), Language::ENGLISH);

    // 更改语言不应影响提供商 / Changing language should not affect provider
    configMgr.setLanguage(Language::CHINESE);
    EXPECT_EQ(configMgr.getProvider(), ProviderType::ANTHROPIC);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
