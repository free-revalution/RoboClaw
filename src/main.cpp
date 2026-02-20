#include <iostream>
#include <string>
#include <vector>
#include <memory>

// 引入自定义模块
#include "utils/logger.h"
#include "storage/config_manager.h"
#include "cli/config_wizard.h"
#include "cli/interactive_mode.h"
#include "llm/llm_provider.h"
#include "llm/anthropic_provider.h"
#include "llm/openai_provider.h"
#include "agent/agent.h"
#include "agent/tool_executor.h"
#include "session/session_manager.h"

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
    CONVERSATION,
    CHAT        // 显式启动对话
};

// CLI选项结构
struct CLIOptions {
    Command command = Command::NONE;
    bool verbose = false;
    string config_action;       // config子命令
    string branch_action;       // branch子命令
    string conversation_action; // conversation子命令
    string argument;            // 附加参数
    string new_conversation;    // 新对话标题
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
    cout << "  chat             启动交互式对话（显式）\n";
    cout << "  --new            创建新对话\n";
    cout << "  branch           分支管理\n";
    cout << "  conversation     对话管理\n";
    cout << "  config           配置管理\n";
    cout << "\n选项:\n";
    cout << "  --help, -h       显示此帮助信息\n";
    cout << "  --version, -v    显示版本信息\n";
    cout << "  --verbose        显示详细日志\n\n";

    cout << "分支命令:\n";
    cout << "  roboclaw branch --list              列出所有分支\n";
    cout << "  roboclaw branch --new <name>        创建新分支\n";
    cout << "  roboclaw branch --switch <name>     切换分支\n\n";

    cout << "配置命令:\n";
    cout << "  roboclaw config --show              显示当前配置\n";
    cout << "  roboclaw config --edit              编辑配置文件\n";
    cout << "  roboclaw config --reset             重置配置\n\n";

    cout << "对话命令:\n";
    cout << "  roboclaw conversation --list        列出所有对话\n";
    cout << "  roboclaw conversation --show <id>   显示对话详情\n";
    cout << "  roboclaw conversation --delete <id> 删除对话\n\n";

    cout << "示例:\n";
    cout << "  roboclaw              # 启动对话\n";
    cout << "  roboclaw chat         # 启动对话\n";
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

        if (arg == "--help" || arg == "-h" || arg == "/help" || arg == "/h") {
            options.command = Command::HELP;
        } else if (arg == "--version" || arg == "-v" || arg == "/version") {
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
                if (next.find("--") == 0 || next.find("-") == 0) {
                    options.config_action = next;
                    while (options.config_action[0] == '-') {
                        options.config_action = options.config_action.substr(1);
                    }
                } else {
                    options.config_action = next;
                }
            }
        } else if (arg == "branch") {
            options.command = Command::BRANCH;
            if (i + 1 < argc) {
                string next = argv[++i];
                if (next.find("--") == 0) {
                    options.branch_action = next.substr(2);
                } else {
                    options.branch_action = next;
                }
            }
        } else if (arg == "conversation") {
            options.command = Command::CONVERSATION;
            if (i + 1 < argc) {
                string next = argv[++i];
                if (next.find("--") == 0) {
                    options.conversation_action = next.substr(2);
                } else {
                    options.conversation_action = next;
                }
            }
        } else if (arg == "chat") {
            options.command = Command::CHAT;
        }
    }

    return options;
}

// 显示配置
void showConfig(ConfigManager& config_mgr) {
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

    // 显示API密钥状态
    cout << "API密钥状态:\n";
    for (const auto& pair : config.providers) {
        const auto& provider = pair.second;
        string status = provider.api_key.empty() ? "未设置" : "已设置";
        string keyPreview = provider.api_key.empty() ? "" :
            provider.api_key.substr(0, 10) + "...";
        cout << "  " << provider.name << ": " << status;
        if (!keyPreview.empty()) {
            cout << " (" << keyPreview << ")";
        }
        cout << "\n";
    }
    cout << endl;
}

// 编辑配置
void editConfig() {
    string configPath = ConfigManager::getConfigPath();
    cout << "配置文件位置: " << configPath << "\n\n";
    cout << "请使用文本编辑器打开上述文件进行编辑。\n";
    #ifdef PLATFORM_MACOS
    cout << "macOS 命令: open " << configPath << "\n";
    #elif defined(PLATFORM_LINUX)
    cout << "Linux 命令: xdg-open " << configPath << " 或 nano " << configPath << "\n";
    #elif defined(PLATFORM_WINDOWS)
    cout << "Windows 命令: notepad " << configPath << "\n";
    #endif
}

// 列出对话
void listConversations(SessionManager& session_mgr) {
    session_mgr.setSessionsDir(".roboclaw/conversations");
    auto sessions = session_mgr.listSessions();

    if (sessions.empty()) {
        cout << "\n暂无对话记录" << endl;
        return;
    }

    cout << "\n对话列表:\n\n";
    for (const auto& session : sessions) {
        cout << "  ID: " << session.id << "\n";
        cout << "  标题: " << session.title << "\n";
        cout << "  消息数: " << session.message_count << "\n";
        cout << "  ────────────────────────────────────────────────\n";
    }
    cout << endl;
}

// 创建LLM提供商
std::unique_ptr<LLMProvider> createLLMProvider(const ConfigManager& config_mgr) {
    const auto& config = config_mgr.getConfig();
    ProviderType providerType = config.default_config.provider;
    string apiKey = config_mgr.getApiKey(providerType);
    string baseUrl = config_mgr.getBaseUrl(providerType);
    string model = config.default_config.model;

    switch (providerType) {
        case ProviderType::ANTHROPIC:
            return std::make_unique<AnthropicProvider>(apiKey, model, baseUrl);
        case ProviderType::OPENAI:
            return std::make_unique<OpenAIProvider>(apiKey, model, baseUrl);
        default:
            cerr << "暂不支持的提供商类型" << endl;
            return nullptr;
    }
}

// 启动交互模式
void startInteractiveMode(ConfigManager& config_mgr) {
    // 加载配置
    if (!config_mgr.load()) {
        cerr << "无法加载配置文件" << endl;
        return;
    }

    // 创建LLM提供商
    auto llmProvider = createLLMProvider(config_mgr);
    if (!llmProvider) {
        cerr << "无法创建LLM提供商" << endl;
        return;
    }

    // 创建工具执行器
    auto toolExecutor = std::make_shared<ToolExecutor>();
    toolExecutor->initialize();

    // 创建Agent
    auto agent = std::make_shared<Agent>(std::move(llmProvider), toolExecutor);

    // 创建会话管理器
    auto sessionManager = std::make_shared<SessionManager>();

    // 创建交互模式
    InteractiveMode interactive(agent, sessionManager, config_mgr);
    interactive.run();
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
                showConfig(config_mgr);
            } else if (options.config_action == "edit") {
                editConfig();
            } else if (options.config_action == "reset") {
                cout << "重置配置功能尚未实现。\n";
            } else {
                showConfig(config_mgr);
            }
            return 0;

        case Command::BRANCH: {
            SessionManager session_mgr;
            auto session = session_mgr.getOrCreateLatestSession();
            if (options.branch_action == "list" || options.branch_action.empty()) {
                auto branches = session->getBranchNames();
                cout << "\n分支列表:\n";
                for (const auto& branch : branches) {
                    cout << "  - " << branch << "\n";
                }
                cout << endl;
            } else if (options.branch_action == "new") {
                auto branch = session->createBranch(session->getCurrentNodeId(), options.argument);
                session_mgr.saveSession(session);
                cout << "已创建分支: " << options.argument << endl;
            } else if (options.branch_action == "switch") {
                // 实现切换逻辑
                cout << "切换分支功能开发中...\n";
            }
            return 0;
        }

        case Command::CONVERSATION: {
            SessionManager session_mgr;
            if (options.conversation_action == "list") {
                listConversations(session_mgr);
            } else if (options.conversation_action == "new") {
                auto session = session_mgr.createSession("新对话");
                cout << "已创建新对话: " << session->getConversationId() << endl;
            } else if (options.conversation_action == "delete") {
                session_mgr.deleteSession(options.argument);
                cout << "已删除对话: " << options.argument << endl;
            } else {
                cout << "对话管理功能开发中...\n";
            }
            return 0;
        }

        case Command::CHAT:
        case Command::NONE:
        default:
            // 启动交互模式
            startInteractiveMode(config_mgr);
            return 0;
    }

    return 0;
}
