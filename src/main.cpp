#include <iostream>
#include <string>
#include <vector>

// 引入自定义模块
#include "utils/logger.h"
#include "storage/config_manager.h"
#include "cli/config_wizard.h"

using namespace std;
using namespace roboclaw;

// 版本信息
const string ROBOCLAW_VERSION = "0.1.0";
const string ROBOCLAW_NAME = "RoboClaw";
const string ROBOCLAW_DESCRIPTION = "C++ AI Agent Framework - 极简AI Agent框架";

// CLI命令枚举
enum class Command {
    NONE,       // 默认对话模式
    HELP,
    VERSION,
    CONFIG,
    BRANCH,
    CONVERSATION
};

// CLI选项结构
struct CLIOptions {
    Command command = Command::NONE;
    bool verbose = false;
    string config_action;      // config子命令: show, edit, reset
    string branch_action;      // branch子命令: list, new, switch, merge, delete
    string conversation_action; // conversation子命令: list, show, delete, export
    string argument;           // 附加参数
};

// 显示横幅
void showBanner() {
    cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << endl;
    cout << "  " << ROBOCLAW_NAME << " v" << ROBOCLAW_VERSION << endl;
    cout << "  " << ROBOCLAW_DESCRIPTION << endl;
    cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << endl;
}

// 显示帮助信息
void showHelp() {
    showBanner();
    cout << "\n用法: roboclaw [命令] [选项]\n";
    cout << "\n命令:\n";
    cout << "  (无)              启动交互式对话\n";
    cout << "  --new             创建新对话\n";
    cout << "  --continue <id>   继续指定ID的对话\n";
    cout << "  branch            管理对话分支\n";
    cout << "  conversation      管理对话\n";
    cout << "  config            管理配置\n";
    cout << "\n选项:\n";
    cout << "  --help, -h        显示此帮助信息\n";
    cout << "  --version, -v     显示版本信息\n";
    cout << "  --verbose         显示详细日志\n\n";

    cout << "分支命令:\n";
    cout << "  roboclaw branch --list              查看所有分支\n";
    cout << "  roboclaw branch --new <name>        创建新分支\n";
    cout << "  roboclaw branch --switch <id>       切换到指定分支\n\n";

    cout << "配置命令:\n";
    cout << "  roboclaw config --show              显示当前配置\n";
    cout << "  roboclaw config --edit              编辑配置文件\n";
    cout << "  roboclaw config --reset             重置为默认配置\n\n";

    cout << "对话命令:\n";
    cout << "  roboclaw conversation --list        列出所有对话\n";
    cout << "  roboclaw conversation --show <id>   显示对话详情\n";
    cout << "  roboclaw conversation --delete <id> 删除对话\n";
    cout << "  roboclaw conversation --export <id> 导出对话为Markdown\n\n";

    cout << "示例:\n";
    cout << "  roboclaw              # 启动对话\n";
    cout << "  roboclaw --new        # 创建新对话\n";
    cout << "  roboclaw config --show # 显示配置\n\n";
}

// 显示版本信息
void showVersion() {
    cout << ROBOCLAW_NAME << " version " << ROBOCLAW_VERSION << endl;
    cout << "Copyright (c) 2025 RoboClaw Contributors\n\n";

    cout << "构建信息:\n";
    cout << "  C++标准: C++" << __cplusplus << "\n";

    #ifdef PLATFORM_MACOS
    cout << "  平台: macOS\n";
    #elif defined(PLATFORM_LINUX)
    cout << "  平台: Linux\n";
    #elif defined(PLATFORM_WINDOWS)
    cout << "  平台: Windows\n";
    #else
    cout << "  平台: Unknown\n";
    #endif
}

// 解析命令行参数
CLIOptions parseArguments(int argc, char* argv[]) {
    CLIOptions options;

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            options.command = Command::HELP;
        } else if (arg == "--version" || arg == "-v") {
            options.command = Command::VERSION;
        } else if (arg == "--verbose") {
            options.verbose = true;
        } else if (arg == "--new") {
            options.command = Command::CONVERSATION;
            options.conversation_action = "new";
        } else if (arg == "--continue") {
            options.command = Command::CONVERSATION;
            options.conversation_action = "continue";
            if (i + 1 < argc) {
                options.argument = argv[++i];
            }
        } else if (arg == "config") {
            options.command = Command::CONFIG;
            if (i + 1 < argc) {
                string next = argv[++i];
                if (next.starts_with("--")) {
                    options.config_action = next.substr(2);
                } else {
                    options.config_action = next;
                }
            }
        } else if (arg == "branch") {
            options.command = Command::BRANCH;
            if (i + 1 < argc) {
                string next = argv[++i];
                if (next.starts_with("--")) {
                    options.branch_action = next.substr(2);
                } else {
                    options.branch_action = next;
                }
            }
        } else if (arg == "conversation") {
            options.command = Command::CONVERSATION;
            if (i + 1 < argc) {
                string next = argv[++i];
                if (next.starts_with("--")) {
                    options.conversation_action = next.substr(2);
                } else {
                    options.conversation_action = next;
                }
            }
        }
    }

    return options;
}

// 显示配置
void showConfig() {
    ConfigManager config_mgr;
    if (!config_mgr.load()) {
        cout << "无法加载配置文件。" << endl;
        return;
    }

    const auto& config = config_mgr.getConfig();

    cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    cout << "  RoboClaw 配置\n";
    cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";

    cout << "默认设置:\n";
    cout << "  提供商: " << ConfigManager::providerToString(config.default_config.provider) << "\n";
    cout << "  模型: " << config.default_config.model << "\n\n";

    cout << "行为设置:\n";
    cout << "  最大重试: " << config.behavior.max_retries << " 次\n";
    cout << "  超时: " << config.behavior.timeout << " 秒\n";
    cout << "  详细日志: " << (config.behavior.verbose ? "开启" : "关闭") << "\n\n";

    cout << "工具设置:\n";
    cout << "  Bash超时: " << config.tools.bash_timeout << " 秒\n";
    cout << "  最大读取: " << config.tools.max_read_size << " MB\n\n";

    // 显示API密钥状态（隐藏实际密钥）
    cout << "API密钥状态:\n";
    for (const auto& pair : config.providers) {
        const auto& provider = pair.second;
        string status = provider.api_key.empty() ? "未设置" : "已设置";
        cout << "  " << provider.name << ": " << status << "\n";
    }
    cout << endl;
}

// 编辑配置
void editConfig() {
    string configPath = ConfigManager::getConfigPath();
    cout << "配置文件位置: " << configPath << "\n";
    cout << "\n提示: 使用文本编辑器打开上述文件进行编辑。\n";
    #ifdef PLATFORM_MACOS
    cout << "macOS 命令: open " << configPath << "\n";
    #elif defined(PLATFORM_LINUX)
    cout << "Linux 命令: xdg-open " << configPath << " 或 nano " << configPath << "\n";
    #elif defined(PLATFORM_WINDOWS)
    cout << "Windows 命令: notepad " << configPath << "\n";
    #endif
}

// 主函数
int main(int argc, char* argv[]) {
    // 解析命令行参数
    CLIOptions options = parseArguments(argc, argv);

    // 检查是否需要首次运行配置
    if (ConfigWizard::needsSetup()) {
        ConfigWizard wizard;
        if (!wizard.run()) {
            return 1;
        }
    }

    // 加载配置
    ConfigManager config_mgr;
    if (config_mgr.load()) {
        const auto& config = config_mgr.getConfig();
        // 设置日志级别
        if (config.behavior.verbose || options.verbose) {
            Logger::getInstance().setLogLevel(LogLevel::DEBUG);
        } else {
            Logger::getInstance().setLogLevel(LogLevel::INFO);
        }

        // 设置日志文件
        Logger::getInstance().setLogFile(ConfigManager::getConfigDir() + "/roboclaw.log");
        Logger::getInstance().setFileOutput(true);

        LOG_INFO("RoboClaw 启动");
    }

    // 处理命令
    switch (options.command) {
        case Command::HELP:
            showHelp();
            return 0;

        case Command::VERSION:
            showVersion();
            return 0;

        case Command::CONFIG:
            if (options.config_action == "show") {
                showConfig();
            } else if (options.config_action == "edit") {
                editConfig();
            } else if (options.config_action == "reset") {
                cout << "重置配置功能尚未实现。\n";
            } else {
                cout << "未知的配置操作: " << options.config_action << "\n";
                cout << "使用 --help 查看帮助。\n";
            }
            return 0;

        case Command::BRANCH:
            cout << "分支管理功能尚未实现。\n";
            cout << "开发中，敬请期待...\n";
            return 0;

        case Command::CONVERSATION:
            cout << "对话管理功能尚未实现。\n";
            cout << "开发中，敬请期待...\n";
            return 0;

        case Command::NONE:
        default:
            // 默认对话模式
            showBanner();
            cout << "\n欢迎使用 RoboClaw！(v" << ROBOCLAW_VERSION << ")\n";
            cout << "\n开发中，核心功能即将推出...\n";
            cout << "\n提示: 使用 --help 查看可用命令\n";
            return 0;
    }

    return 0;
}
