// ConfigWizard实现

#include "config_wizard.h"
#include "../utils/logger.h"
#include <iostream>
#include <limits>
#include <algorithm>
#include <map>

namespace roboclaw {

// 本地化文本映射
static const std::map<std::string, std::map<Language, std::string>> i18n = {
    {"welcome_title", {{Language::CHINESE, "欢迎使用 RoboClaw！"}, {Language::ENGLISH, "Welcome to RoboClaw!"}}},
    {"welcome_subtitle", {{Language::CHINESE, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"}, {Language::ENGLISH, "============================================================"}}},
    {"first_run", {{Language::CHINESE, "\n这是您第一次运行 RoboClaw，我们需要进行一些基本配置。\n配置将保存在: "}, {Language::ENGLISH, "\nThis is your first time running RoboClaw. Let's do some basic setup.\nConfiguration will be saved to: "}}},
    {"select_language", {{Language::CHINESE, "\n请选择语言 / Please select language:\n\n  1. 简体中文 (Simplified Chinese)\n  2. English\n"}, {Language::ENGLISH, "\nPlease select language:\n\n  1. Simplified Chinese (简体中文)\n  2. English\n"}}},
    {"language_prompt", {{Language::CHINESE, "\n请输入选项 (1-2): "}, {Language::ENGLISH, "\nPlease enter option (1-2): "}}},
    {"select_provider", {{Language::CHINESE, "\n请选择默认的 LLM 提供商:\n\n"}, {Language::ENGLISH, "\nPlease select default LLM provider:\n\n"}}},
    {"provider_1", {{Language::CHINESE, "  1. Anthropic (Claude)      [推荐]\n"}, {Language::ENGLISH, "  1. Anthropic (Claude)      [Recommended]\n"}}},
    {"provider_2", {{Language::CHINESE, "  2. OpenAI (GPT)\n"}, {Language::ENGLISH, "  2. OpenAI (GPT)\n"}}},
    {"provider_3", {{Language::CHINESE, "  3. Google Gemini\n"}, {Language::ENGLISH, "  3. Google Gemini\n"}}},
    {"provider_4", {{Language::CHINESE, "  4. 深度求索 (DeepSeek)\n"}, {Language::ENGLISH, "  4. DeepSeek\n"}}},
    {"provider_5", {{Language::CHINESE, "  5. 字节豆包 (Doubao)\n"}, {Language::ENGLISH, "  5. ByteDance Doubao\n"}}},
    {"provider_6", {{Language::CHINESE, "  6. 阿里通义千问 (Qwen)\n"}, {Language::ENGLISH, "  6. Alibaba Qwen\n"}}},
    {"provider_prompt", {{Language::CHINESE, "\n请输入选项 (1-6): "}, {Language::ENGLISH, "\nPlease enter option (1-6): "}}},
    {"enter_api_key", {{Language::CHINESE, "\n请输入 "}, {Language::ENGLISH, "\nPlease enter "}}},
    {"api_key_suffix", {{Language::CHINESE, " 的 API 密钥: "}, {Language::ENGLISH, " API key: "}}},
    {"api_key_empty", {{Language::CHINESE, "API 密钥不能为空，请重新输入。"}, {Language::ENGLISH, "API key cannot be empty. Please try again."}}},
    {"api_key_warning", {{Language::CHINESE, "警告: API 密钥看起来不完整，是否继续？(y/n): "}, {Language::ENGLISH, "Warning: API key appears incomplete. Continue? (y/n): "}}},
    {"select_model", {{Language::CHINESE, "\n请选择默认模型:\n\n"}, {Language::ENGLISH, "\nPlease select default model:\n\n"}}},
    {"recommended", {{Language::CHINESE, "  [推荐]"}, {Language::ENGLISH, "  [Recommended]"}}},
    {"model_prompt", {{Language::CHINESE, "\n请输入选项: "}, {Language::ENGLISH, "\nPlease enter option: "}}},
    {"config_summary", {{Language::CHINESE, "\n配置摘要:\n\n"}, {Language::ENGLISH, "\nConfiguration Summary:\n\n"}}},
    {"provider_label", {{Language::CHINESE, "  提供商: "}, {Language::ENGLISH, "  Provider: "}}},
    {"model_label", {{Language::CHINESE, "  模型: "}, {Language::ENGLISH, "  Model: "}}},
    {"api_key_label", {{Language::CHINESE, "  API密钥: "}, {Language::ENGLISH, "  API Key: "}}},
    {"hidden", {{Language::CHINESE, "... (已隐藏)\n"}, {Language::ENGLISH, "... (hidden)\n"}}},
    {"confirm_save", {{Language::CHINESE, "\n确认保存配置？(y/n): "}, {Language::ENGLISH, "\nConfirm save configuration? (y/n): "}}},
    {"saving_config", {{Language::CHINESE, "\n正在保存配置..."}, {Language::ENGLISH, "\nSaving configuration..."}}},
    {"save_error", {{Language::CHINESE, "\n错误: 无法保存配置文件"}, {Language::ENGLISH, "\nError: Unable to save configuration file"}}},
    {"check_permissions", {{Language::CHINESE, "请检查目录权限: "}, {Language::ENGLISH, "Please check directory permissions: "}}},
    {"config_complete_title", {{Language::CHINESE, "  配置完成！"}, {Language::ENGLISH, "  Configuration Complete!"}}},
    {"config_saved", {{Language::CHINESE, "\n配置已保存到: "}, {Language::ENGLISH, "\nConfiguration saved to: "}}},
    {"ready_to_use", {{Language::CHINESE, "\n现在可以开始使用 RoboClaw 了！\n"}, {Language::ENGLISH, "\nYou are now ready to use RoboClaw!\n"}}},
    {"tips", {{Language::CHINESE, "提示:\n"}, {Language::ENGLISH, "Tips:\n"}}},
    {"tip_run", {{Language::CHINESE, "  - 运行 'roboclaw' 启动对话\n"}, {Language::ENGLISH, "  - Run 'roboclaw' to start conversation\n"}}},
    {"tip_help", {{Language::CHINESE, "  - 运行 'roboclaw --help' 查看所有命令\n"}, {Language::ENGLISH, "  - Run 'roboclaw --help' to see all commands\n"}}},
    {"tip_edit", {{Language::CHINESE, "  - 运行 'roboclaw config --edit' 编辑配置\n\n"}, {Language::ENGLISH, "  - Run 'roboclaw config --edit' to edit configuration\n\n"}}},
    {"config_cancelled", {{Language::CHINESE, "\n配置已取消。"}, {Language::ENGLISH, "\nConfiguration cancelled."}}},
    {"enter_number", {{Language::CHINESE, "请输入有效的数字。"}, {Language::ENGLISH, "Please enter a valid number."}}},
    {"number_range", {{Language::CHINESE, "请输入 "}, {Language::ENGLISH, "Please enter a number between "}}},
    {"number_range_suffix", {{Language::CHINESE, " 到 "}, {Language::ENGLISH, " and "}}},
};

#ifdef PLATFORM_WINDOWS
#include <windows.h>
void clearScreenWrapper() { system("cls"); }
#else
void clearScreenWrapper() { system("clear"); }
#endif

ConfigWizard::ConfigWizard() {
}

bool ConfigWizard::needsSetup() {
    return !ConfigManager::configExists();
}

std::string ConfigWizard::getText(const std::string& key) const {
    auto it = i18n.find(key);
    if (it != i18n.end()) {
        auto langIt = it->second.find(language_);
        if (langIt != it->second.end()) {
            return langIt->second;
        }
        // Fallback to Chinese if current language not found
        auto fallbackIt = it->second.find(Language::CHINESE);
        if (fallbackIt != it->second.end()) {
            return fallbackIt->second;
        }
    }
    return key;  // Return key if not found
}

Language ConfigWizard::selectLanguage() {
    while (true) {
        std::cout << getText("select_language");
        int choice = readInt(getText("language_prompt"), 1, 2);

        switch (choice) {
            case 1:
                language_ = Language::CHINESE;
                config_manager_.setLanguage(language_);
                return language_;
            case 2:
                language_ = Language::ENGLISH;
                config_manager_.setLanguage(language_);
                return language_;
        }
    }
}

void ConfigWizard::showWelcome() {
    clearScreen();
    std::cout << getText("welcome_subtitle") << std::endl;
    std::cout << "  " << getText("welcome_title") << std::endl;
    std::cout << getText("welcome_subtitle") << std::endl;
    std::cout << getText("first_run") << ConfigManager::getConfigPath() << "\n" << std::endl;
}

ProviderType ConfigWizard::selectProvider() {
    while (true) {
        printSeparator();
        std::cout << getText("select_provider");
        std::cout << getText("provider_1");
        std::cout << getText("provider_2");
        std::cout << getText("provider_3");
        std::cout << getText("provider_4");
        std::cout << getText("provider_5");
        std::cout << getText("provider_6");

        int choice = readInt(getText("provider_prompt"), 1, 6);

        switch (choice) {
            case 1: return ProviderType::ANTHROPIC;
            case 2: return ProviderType::OPENAI;
            case 3: return ProviderType::GEMINI;
            case 4: return ProviderType::DEEPSEEK;
            case 5: return ProviderType::DOUBAO;
            case 6: return ProviderType::QWEN;
        }
    }
}

std::string ConfigWizard::inputApiKey(ProviderType provider) {
    std::string providerName = ConfigManager::providerToString(provider);
    std::string prompt = getText("enter_api_key") + providerName + getText("api_key_suffix");

    std::string apiKey;
    while (true) {
        apiKey = readLine(prompt);

        if (apiKey.empty()) {
            std::cout << getText("api_key_empty") << std::endl;
            continue;
        }

        // 简单验证
        if (apiKey.length() < 10) {
            std::cout << getText("api_key_warning");
            std::string confirm;
            std::getline(std::cin, confirm);
            if (confirm != "y" && confirm != "Y" && confirm != "yes") {
                continue;
            }
        }

        return apiKey;
    }
}

std::string ConfigWizard::selectModel(ProviderType provider) {
    const auto& config = config_manager_.getConfig();
    auto it = config.providers.find(provider);
    if (it == config.providers.end() || it->second.models.empty()) {
        return "default";
    }

    const auto& models = it->second.models;

    while (true) {
        printSeparator();
        std::cout << getText("select_model");

        for (size_t i = 0; i < models.size(); ++i) {
            std::cout << "  " << (i + 1) << ". " << models[i];
            if (i == 0) std::cout << "  " << getText("recommended");
            std::cout << "\n";
        }

        int choice = readInt(getText("model_prompt"), 1, static_cast<int>(models.size()));
        return models[choice - 1];
    }
}

bool ConfigWizard::confirmConfig() {
    printSeparator();
    std::cout << getText("config_summary");

    const auto& config = config_manager_.getConfig();
    std::cout << getText("provider_label") << ConfigManager::providerToString(config.default_config.provider) << "\n";
    std::cout << getText("model_label") << config.default_config.model << "\n";
    std::cout << getText("api_key_label") << config.providers.at(config.default_config.provider).api_key.substr(0, 10)
              << "..." << getText("hidden");

    std::string confirm = readLine(getText("confirm_save"));
    return (confirm == "y" || confirm == "Y" || confirm == "yes");
}

bool ConfigWizard::saveConfig() {
    std::cout << getText("saving_config") << std::endl;

    if (config_manager_.save()) {
        return true;
    }

    std::cerr << getText("save_error") << std::endl;
    std::cerr << getText("check_permissions") << ConfigManager::getConfigDir() << std::endl;
    return false;
}

void ConfigWizard::showComplete() {
    clearScreen();
    std::cout << "\n" << getText("welcome_subtitle") << std::endl;
    std::cout << "  " << getText("config_complete_title") << std::endl;
    std::cout << getText("welcome_subtitle") << std::endl;
    std::cout << getText("config_saved") << ConfigManager::getConfigPath() << "\n";
    std::cout << getText("ready_to_use");
    std::cout << getText("tips");
    std::cout << getText("tip_run");
    std::cout << getText("tip_help");
    std::cout << getText("tip_edit");
}

std::string ConfigWizard::readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    std::getline(std::cin, line);
    return line;
}

int ConfigWizard::readInt(const std::string& prompt, int min, int max) {
    while (true) {
        std::string input = readLine(prompt);
        try {
            int value = std::stoi(input);
            if (value >= min && value <= max) {
                return value;
            }
            std::cout << getText("number_range") << min << getText("number_range_suffix") << max << "." << std::endl;
        } catch (...) {
            std::cout << getText("enter_number") << std::endl;
        }
    }
}

void ConfigWizard::clearScreen() {
    clearScreenWrapper();
}

void ConfigWizard::printSeparator() {
    std::cout << "\n" << getText("welcome_subtitle") << std::endl;
}

bool ConfigWizard::run() {
    showWelcome();

    // 选择语言
    selectLanguage();

    // 选择提供商
    ProviderType provider = selectProvider();
    config_manager_.setProvider(provider);

    // 输入API密钥
    std::string apiKey = inputApiKey(provider);
    config_manager_.setApiKey(provider, apiKey);

    // 选择模型
    std::string model = selectModel(provider);
    config_manager_.setModel(model);

    // 确认配置
    if (!confirmConfig()) {
        std::cout << getText("config_cancelled") << std::endl;
        return false;
    }

    // 保存配置
    if (!saveConfig()) {
        return false;
    }

    // 显示完成信息
    showComplete();

    return true;
}

} // namespace roboclaw
