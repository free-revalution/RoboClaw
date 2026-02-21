// 配置向导 - ConfigWizard
// 首次运行时引导用户配置RoboClaw

#ifndef ROBOCLAW_CLI_CONFIG_WIZARD_H
#define ROBOCLAW_CLI_CONFIG_WIZARD_H

#include <string>
#include <functional>
#include "../storage/config_manager.h"

namespace roboclaw {

// 配置向导类
class ConfigWizard {
public:
    ConfigWizard();
    ~ConfigWizard() = default;

    // 运行配置向导
    bool run();

    // 检查是否需要运行向导
    static bool needsSetup();

private:
    // 选择语言
    Language selectLanguage();

    // 显示欢迎信息
    void showWelcome();

    // 选择提供商
    ProviderType selectProvider();

    // 输入API密钥
    std::string inputApiKey(ProviderType provider);

    // 选择模型
    std::string selectModel(ProviderType provider);

    // 确认配置
    bool confirmConfig();

    // 保存配置
    bool saveConfig();

    // 显示完成信息
    void showComplete();

    // 辅助方法
    std::string readLine(const std::string& prompt);
    int readInt(const std::string& prompt, int min, int max);
    void clearScreen();
    void printSeparator();

    // 获取本地化文本
    std::string getText(const std::string& key) const;

    ConfigManager config_manager_;
    Language language_ = Language::CHINESE;
};

} // namespace roboclaw

#endif // ROBOCLAW_CLI_CONFIG_WIZARD_H
