// ConfigWizard实现

#include "config_wizard.h"
#include "../utils/logger.h"
#include <iostream>
#include <limits>
#include <algorithm>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
void clearScreen() { system("cls"); }
#else
void clearScreen() { system("clear"); }
#endif

namespace roboclaw {

ConfigWizard::ConfigWizard() {
}

bool ConfigWizard::needsSetup() {
    return !ConfigManager::configExists();
}

void ConfigWizard::showWelcome() {
    clearScreen();
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "  欢迎使用 RoboClaw！" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "\n这是您第一次运行 RoboClaw，我们需要进行一些基本配置。\n"
              << "配置将保存在: " << ConfigManager::getConfigPath() << "\n" << std::endl;
}

ProviderType ConfigWizard::selectProvider() {
    while (true) {
        printSeparator();
        std::cout << "\n请选择默认的 LLM 提供商:\n\n";
        std::cout << "  1. Anthropic (Claude)      [推荐]\n";
        std::cout << "  2. OpenAI (GPT)\n";
        std::cout << "  3. Google Gemini\n";
        std::cout << "  4. 深度求索 (DeepSeek)\n";
        std::cout << "  5. 字节豆包 (Doubao)\n";
        std::cout << "  6. 阿里通义千问 (Qwen)\n";

        int choice = readInt("\n请输入选项 (1-6): ", 1, 6);

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
    std::string prompt = "\n请输入 " + providerName + " 的 API 密钥: ";

    std::string apiKey;
    while (true) {
        apiKey = readLine(prompt);

        if (apiKey.empty()) {
            std::cout << "API 密钥不能为空，请重新输入。" << std::endl;
            continue;
        }

        // 简单验证
        if (apiKey.length() < 10) {
            std::cout << "警告: API 密钥看起来不完整，是否继续？(y/n): ";
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
        std::cout << "\n请选择默认模型:\n\n";

        for (size_t i = 0; i < models.size(); ++i) {
            std::cout << "  " << (i + 1) << ". " << models[i];
            if (i == 0) std::cout << "  [推荐]";
            std::cout << "\n";
        }

        int choice = readInt("\n请输入选项: ", 1, static_cast<int>(models.size()));
        return models[choice - 1];
    }
}

bool ConfigWizard::confirmConfig() {
    printSeparator();
    std::cout << "\n配置摘要:\n\n";

    const auto& config = config_manager_.getConfig();
    std::cout << "  提供商: " << ConfigManager::providerToString(config.default_config.provider) << "\n";
    std::cout << "  模型: " << config.default_config.model << "\n";
    std::cout << "  API密钥: " << config.providers.at(config.default_config.provider).api_key.substr(0, 10)
              << "..." << " (已隐藏)\n";

    std::string confirm = readLine("\n确认保存配置？(y/n): ");
    return (confirm == "y" || confirm == "Y" || confirm == "yes");
}

bool ConfigWizard::saveConfig() {
    std::cout << "\n正在保存配置..." << std::endl;

    if (config_manager_.save()) {
        return true;
    }

    std::cerr << "\n错误: 无法保存配置文件" << std::endl;
    std::cerr << "请检查目录权限: " << ConfigManager::getConfigDir() << std::endl;
    return false;
}

void ConfigWizard::showComplete() {
    clearScreen();
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "  配置完成！" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "\n配置已保存到: " << ConfigManager::getConfigPath() << "\n";
    std::cout << "\n现在可以开始使用 RoboClaw 了！\n";
    std::cout << "\n提示:\n";
    std::cout << "  - 运行 'roboclaw' 启动对话\n";
    std::cout << "  - 运行 'roboclaw --help' 查看所有命令\n";
    std::cout << "  - 运行 'roboclaw config --edit' 编辑配置\n\n";
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
            std::cout << "请输入 " << min << " 到 " << max << " 之间的数字。" << std::endl;
        } catch (...) {
            std::cout << "请输入有效的数字。" << std::endl;
        }
    }
}

void ConfigWizard::clearScreen() {
    ::clearScreen();
}

void ConfigWizard::printSeparator() {
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
}

bool ConfigWizard::run() {
    showWelcome();

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
        std::cout << "\n配置已取消。" << std::endl;
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
