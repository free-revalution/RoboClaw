#include <iostream>
#include <string>
#include <vector>
#include <memory>

// 引入自定义模块
#include "utils/logger.h"
#include "storage/config_manager.h"
#include "cli/config_wizard.h"
#include "cli/interactive_mode.h"
#include "cli/skill_commands.h"
#include "cli/link_command.h"
#include "llm/llm_provider.h"
#include "llm/anthropic_provider.h"
#include "llm/openai_provider.h"
#include "agent/agent.h"
#include "agent/tool_executor.h"
#include "session/session_manager.h"
#include "skills/skill_registry.h"
#include "skills/skill_executor.h"
#include "optimization/token_optimizer.h"
#include "optimization/token_budget.h"
#include "hal/hardware_config.h"

using namespace std;
using namespace roboclaw;

// 获取内置skills目录路径
string getBuiltinSkillsDir() {
    // 首先尝试相对于可执行文件的路径
    // 对于开发环境，可执行文件在 build/roboclaw
    // 技能文件在 skills/builtin
    // 所以需要查找项目根目录

    // 方法1: 检查相对路径
    if (std::filesystem::exists("skills/builtin")) {
        return "skills/builtin";
    }

    // 方法2: 检查上一级目录（从 build 目录运行）
    if (std::filesystem::exists("../skills/builtin")) {
        return "../skills/builtin";
    }

    // 方法3: 使用安装路径
    string installPath = "/usr/local/share/roboclaw/skills/builtin";
    if (std::filesystem::exists(installPath)) {
        return installPath;
    }

    // 方法4: 使用用户目录下的安装路径
    const char* home = getenv("HOME");
    if (home) {
        string userInstallPath = string(home) + "/.local/share/roboclaw/skills/builtin";
        if (std::filesystem::exists(userInstallPath)) {
            return userInstallPath;
        }
    }

    // 默认返回当前目录下的 skills/builtin
    // 如果不存在，会在加载时显示警告
    return "skills/builtin";
}

// 版本信息
const string ROBOCLAW_VERSION = "1.0.0";
const string ROBOCLAW_NAME = "RoboClaw";
const string ROBOCLAW_DESCRIPTION = "C++ AI Agent Framework with Browser Automation - AI Agent框架与浏览器自动化";

// CLI命令枚举
enum class Command {
    NONE,       // 默认对话模式
    HELP,
    VERSION,
    CONFIG,
    BRANCH,
    CONVERSATION,
    SKILL,      // 技能管理
    HARDWARE,   // 硬件管理
    LINK,       // 社交平台连接
    CHAT        // 显式启动对话
};

// CLI选项结构
struct CLIOptions {
    Command command = Command::NONE;
    bool verbose = false;
    string config_action;       // config子命令
    string branch_action;       // branch子命令
    string conversation_action; // conversation子命令
    string skill_action;        // skill子命令
    string hardware_action;     // hardware子命令
    string link_action;         // link子命令
    string argument;            // 附加参数
    string new_conversation;    // 新对话标题
};

// 显示横幅
void showBanner() {
    cout << "==================================================" << endl;
    cout << "  " << ROBOCLAW_NAME << " v" << ROBOCLAW_VERSION << endl;
    cout << "  " << ROBOCLAW_DESCRIPTION << endl;
    cout << "==================================================" << endl;
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
    cout << "  skill            技能管理\n";
    cout << "  hardware         硬件管理\n";
    cout << "  link             社交平台连接\n";
    cout << "  agent            Agent管理 (新增)\n";
    cout << "  browser          浏览器自动化 (新增)\n";
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

    cout << "技能命令:\n";
    cout << "  roboclaw skill --list              列出所有技能\n";
    cout << "  roboclaw skill --show <name>       显示技能详情\n";
    cout << "  roboclaw skill --install <file>    安装技能\n";
    cout << "  roboclaw skill --uninstall <name>  卸载技能\n";
    cout << "  roboclaw skill --create <name>     创建新技能\n\n";

    cout << "Agent命令 (新增):\n";
    cout << "  roboclaw agent --list             列出本地已安装的Agents\n";
    cout << "  roboclaw agent --show <name>      显示Agent详情\n";
    cout << "  roboclaw agent --launch <name>     启动指定Agent\n\n";

    cout << "浏览器命令 (新增):\n";
    cout << "  roboclaw browser --open           打开浏览器\n";
    cout << "  roboclaw browser --screenshot     截图\n";
    cout << "  roboclaw browser --navigate <url> 导航到URL\n";
    cout << "  roboclaw browser --click <selector> 点击元素\n";
    cout << "  roboclaw browser --type <text>     输入文本\n\n";

    cout << "硬件命令:\n";
    cout << "  roboclaw hardware --list          列出所有硬件\n";
    cout << "  roboclaw hardware --test          测试硬件连接\n\n";

    cout << "连接命令:\n";
    cout << "  roboclaw link --list             列出可用平台\n";
    cout << "  roboclaw link --connect <platform> 连接到平台\n";
    cout << "  roboclaw link --status           显示连接状态\n\n";

    cout << "示例:\n";
    cout << "  roboclaw              # 启动对话\n";
    cout << "  roboclaw chat         # 启动对话\n";
    cout << "  roboclaw --new        # 创建新对话\n";
    cout << "  roboclaw config --show # 显示配置\n";
    cout << "  roboclaw agent --list # 列出Agents\n\n";
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
        } else if (arg == "skill") {
            options.command = Command::SKILL;
            if (i + 1 < argc) {
                string next = argv[++i];
                if (next.find("--") == 0) {
                    options.skill_action = next.substr(2);
                } else {
                    options.skill_action = next;
                }
            }
        } else if (arg == "hardware") {
            options.command = Command::HARDWARE;
            if (i + 1 < argc) {
                string next = argv[++i];
                if (next.find("--") == 0) {
                    options.hardware_action = next.substr(2);
                } else {
                    options.hardware_action = next;
                }
            }
        } else if (arg == "link") {
            options.command = Command::LINK;
            if (i + 1 < argc) {
                string next = argv[++i];
                if (next.find("--") == 0) {
                    options.link_action = next.substr(2);
                } else {
                    options.link_action = next;
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

    cout << "\n==================================================\n";
    cout << "  RoboClaw 配置\n";
    cout << "==================================================\n\n";

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

    cout << "技能设置:\n";
    cout << "  本地目录: " << config.skills.local_skills_dir << "\n";
    cout << "  自动更新: " << (config.skills.auto_update ? "开启" : "关闭") << "\n";
    cout << "  更新间隔: " << config.skills.update_interval_hours << " 小时\n\n";

    cout << "Token优化:\n";
    cout << "  历史压缩: " << (config.optimization.enable_compression ? "开启" : "关闭") << "\n";
    cout << "  压缩阈值: " << config.optimization.compression_threshold << " tokens\n";
    cout << "  目标预算: " << config.optimization.target_budget << " tokens\n";
    cout << "  提示词缓存: " << (config.optimization.enable_prompt_caching ? "开启" : "关闭") << "\n\n";

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

// 显示硬件帮助信息
void showHardwareHelp() {
    cout << "\n硬件命令:\n\n";
    cout << "  roboclaw hardware --list          列出所有已配置的硬件\n";
    cout << "  roboclaw hardware --test          测试硬件连接状态\n";
    cout << "\n示例:\n";
    cout << "  roboclaw hardware --list          # 显示所有硬件\n";
    cout << "  roboclaw hardware --test          # 测试硬件连接\n\n";
}

// 处理硬件命令
int handleHardwareCommand(const std::string& action, const std::string& argument) {
    using namespace roboclaw::hal;

    // 查找硬件配置文件
    std::string configPath = "config/hardware.json";

    // 尝试多个可能的配置文件位置
    if (!std::filesystem::exists(configPath)) {
        if (std::filesystem::exists("../config/hardware.json")) {
            configPath = "../config/hardware.json";
        } else if (std::filesystem::exists("/usr/local/etc/roboclaw/hardware.json")) {
            configPath = "/usr/local/etc/roboclaw/hardware.json";
        }
    }

    HardwareConfig hwConfig;

    if (action == "list" || action.empty()) {
        // 列出所有硬件
        cout << "\n==================================================\n";
        cout << "  硬件配置列表\n";
        cout << "==================================================\n\n";

        if (std::filesystem::exists(configPath)) {
            if (!hwConfig.loadFromFile(configPath)) {
                cout << "错误: 无法加载硬件配置文件: " << configPath << "\n\n";
                return 1;
            }

            // 显示电机配置
            auto motorNames = hwConfig.getMotorNames();
            if (!motorNames.empty()) {
                cout << "电机 (" << motorNames.size() << "):\n";
                for (const auto& name : motorNames) {
                    auto motorConfig = hwConfig.getMotorConfig(name);
                    string type = motorConfig.value("type", "unknown");
                    cout << "  - " << name << " (" << type << ")\n";
                }
                cout << "\n";
            } else {
                cout << "电机: 未配置\n\n";
            }

            // 显示传感器配置
            auto sensorNames = hwConfig.getSensorNames();
            if (!sensorNames.empty()) {
                cout << "传感器 (" << sensorNames.size() << "):\n";
                for (const auto& name : sensorNames) {
                    auto sensorConfig = hwConfig.getSensorConfig(name);
                    string type = sensorConfig.value("type", "unknown");
                    cout << "  - " << name << " (" << type << ")\n";
                }
                cout << "\n";
            } else {
                cout << "传感器: 未配置\n\n";
            }

            cout << "配置文件: " << configPath << "\n\n";
        } else {
            cout << "未找到硬件配置文件。\n\n";
            cout << "预期位置:\n";
            cout << "  - " << "config/hardware.json\n";
            cout << "  - " << "/usr/local/etc/roboclaw/hardware.json\n\n";
            cout << "请创建硬件配置文件以继续。\n\n";
            return 1;
        }

    } else if (action == "test") {
        // 测试硬件连接
        cout << "\n==================================================\n";
        cout << "  硬件连接测试\n";
        cout << "==================================================\n\n";

        if (!std::filesystem::exists(configPath)) {
            cout << "错误: 未找到硬件配置文件: " << configPath << "\n\n";
            cout << "请先创建硬件配置文件。\n\n";
            return 1;
        }

        if (!hwConfig.loadFromFile(configPath)) {
            cout << "错误: 无法加载硬件配置文件\n\n";
            return 1;
        }

        cout << "硬件配置文件加载成功\n\n";

        // 测试电机
        auto motorNames = hwConfig.getMotorNames();
        if (!motorNames.empty()) {
            cout << "电机配置验证:\n";
            for (const auto& name : motorNames) {
                if (hwConfig.hasMotor(name)) {
                    auto motorConfig = hwConfig.getMotorConfig(name);
                    string type = motorConfig.value("type", "unknown");
                    cout << "  [OK] " << name << " (" << type << ")\n";
                }
            }
            cout << "\n";
        }

        // 测试传感器
        auto sensorNames = hwConfig.getSensorNames();
        if (!sensorNames.empty()) {
            cout << "传感器配置验证:\n";
            for (const auto& name : sensorNames) {
                if (hwConfig.hasSensor(name)) {
                    auto sensorConfig = hwConfig.getSensorConfig(name);
                    string type = sensorConfig.value("type", "unknown");
                    cout << "  [OK] " << name << " (" << type << ")\n";
                }
            }
            cout << "\n";
        }

        cout << "注意: 配置文件验证通过。\n";
        cout << "实际硬件连接测试需要相应的硬件抽象层实现。\n\n";

    } else if (action == "help" || action == "--help" || action == "-h") {
        showHardwareHelp();
    } else {
        cout << "未知硬件命令: " << action << "\n";
        showHardwareHelp();
        return 1;
    }

    return 0;
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
        cout << "  ----------------------------------------------\n";
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

    // 检查API密钥是否已设置
    if (apiKey.empty()) {
        cerr << "错误: API密钥未设置。请运行 'roboclaw config --edit' 配置API密钥。" << endl;
        cerr << "当前提供商: " << ConfigManager::providerToString(providerType) << endl;
        return nullptr;
    }

    switch (providerType) {
        case ProviderType::ANTHROPIC:
            return std::make_unique<AnthropicProvider>(apiKey, model, baseUrl);
        case ProviderType::OPENAI:
        case ProviderType::GEMINI:
        case ProviderType::DEEPSEEK:
        case ProviderType::DOUBAO:
        case ProviderType::QWEN:
            // 使用OpenAI兼容接口（支持大多数国产大模型）
            return std::make_unique<OpenAIProvider>(apiKey, model, baseUrl);
        default:
            cerr << "暂不支持的提供商类型: " << ConfigManager::providerToString(providerType) << endl;
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

    const auto& config = config_mgr.getConfig();

    // 创建LLM提供商
    auto llmProvider = createLLMProvider(config_mgr);
    if (!llmProvider) {
        cerr << "无法创建LLM提供商" << endl;
        return;
    }

    // 创建工具执行器
    auto toolExecutor = std::make_unique<ToolExecutor>();
    toolExecutor->initialize();

    // 创建Agent
    auto agent = std::make_shared<Agent>(std::move(llmProvider), std::move(toolExecutor));

    // 创建Token优化器
    std::shared_ptr<TokenOptimizer> tokenOptimizer;
    if (config.optimization.enable_compression) {
        tokenOptimizer = std::make_shared<TokenOptimizer>();
        agent->setTokenOptimizer(tokenOptimizer);
        agent->enableTokenOptimization(true);
        LOG_INFO("Token优化已启用");
    }

    // 创建Token预算管理
    auto tokenBudget = std::make_shared<TokenBudget>();
    tokenBudget->setBudget(config.optimization.target_budget);
    if (tokenOptimizer) {
        tokenBudget->setOptimizer(tokenOptimizer);
    }
    agent->setTokenBudget(tokenBudget);

    // 创建技能注册表并加载技能
    auto skillRegistry = std::make_shared<SkillRegistry>();
    skillRegistry->loadSkillsFromDirectory(getBuiltinSkillsDir());
    skillRegistry->loadSkillsFromDirectory(config.skills.local_skills_dir);
    LOG_INFO("已加载 " + std::to_string(skillRegistry->getAllSkills().size()) + " 个技能");

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

        case Command::SKILL: {
            auto skillRegistry = std::make_shared<SkillRegistry>();
            SkillCommands skillCmd(skillRegistry, config_mgr);
            skillCmd.reloadSkills();

            if (options.skill_action == "list" || options.skill_action.empty()) {
                return skillCmd.listSkills();
            } else if (options.skill_action == "show") {
                return skillCmd.showSkill(options.argument);
            } else if (options.skill_action == "install") {
                return skillCmd.installSkill(options.argument);
            } else if (options.skill_action == "uninstall") {
                return skillCmd.uninstallSkill(options.argument);
            } else if (options.skill_action == "create") {
                return skillCmd.createSkill(options.argument);
            } else if (options.skill_action == "reload") {
                return skillCmd.reloadSkills();
            } else {
                cout << "未知技能命令: " << options.skill_action << "\n";
                return 1;
            }
        }

        case Command::HARDWARE:
            return handleHardwareCommand(options.hardware_action, options.argument);

        case Command::LINK: {
            using namespace roboclaw::cli;
            LinkCommand linkCmd;

            if (options.link_action == "list" || options.link_action.empty()) {
                auto platforms = linkCmd.getAvailablePlatforms();
                cout << "\n可用平台:\n";
                for (size_t i = 0; i < platforms.size(); i++) {
                    cout << "  " << (i + 1) << ". " << platforms[i].name
                         << " (" << platforms[i].description << ")";
                    if (!platforms[i].enabled) {
                        cout << " [未启用]";
                    }
                    cout << "\n";
                }
                cout << "\n使用 'roboclaw link --connect <platform_id>' 连接到平台\n";
                cout << "使用 'roboclaw link --status' 查看连接状态\n\n";
                return 0;
            } else if (options.link_action == "status") {
                cout << "\n" << linkCmd.getConnectionStatus() << "\n\n";
                return 0;
            } else if (options.link_action == "connect") {
                string platform_id = options.argument;
                if (platform_id.empty()) {
                    cout << "请指定平台ID (例如: telegram)\n";
                    cout << "使用 'roboclaw link --list' 查看可用平台\n\n";
                    return 1;
                }

                cout << "\n连接到平台: " << platform_id << "\n";
                cout << "请输入 Bot Token: ";
                string token;
                cin >> token;

                nlohmann::json config = {{"bot_token", token}};
                if (linkCmd.connectToPlatform(platform_id, config)) {
                    cout << "连接成功！\n\n";
                    return 0;
                } else {
                    cout << "连接失败，请检查配置。\n\n";
                    return 1;
                }
            } else if (options.link_action == "help" || options.link_action == "--help" || options.link_action == "-h") {
                cout << "\n连接命令:\n\n";
                cout << "  roboclaw link --list             列出可用平台\n";
                cout << "  roboclaw link --connect <platform> 连接到平台\n";
                cout << "  roboclaw link --status           显示连接状态\n\n";
                cout << "示例:\n";
                cout << "  roboclaw link --list             # 显示所有平台\n";
                cout << "  roboclaw link --connect telegram # 连接到Telegram\n";
                cout << "  roboclaw link --status           # 查看连接状态\n\n";
                return 0;
            } else {
                cout << "未知连接命令: " << options.link_action << "\n";
                cout << "使用 'roboclaw link --help' 查看帮助\n\n";
                return 1;
            }
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
