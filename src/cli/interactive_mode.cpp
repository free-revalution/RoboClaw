// InteractiveMode实现

#include "interactive_mode.h"
#include "link_command.h"
#include "../utils/logger.h"
#include "../utils/terminal.h"  // NEW
#include "../storage/config_manager.h"
#include <iostream>
#include <sstream>
#include <algorithm>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace roboclaw {

InteractiveMode::InteractiveMode(std::shared_ptr<Agent> agent,
                                 std::shared_ptr<SessionManager> sessionMgr,
                                 const ConfigManager& configMgr)
    : agent_(agent)
    , session_manager_(sessionMgr)
    , config_manager_(configMgr)
    , should_exit_(false) {
}

void InteractiveMode::run() {
    showWelcome();
    showBanner();

    // 设置会话目录
    session_manager_->setSessionsDir(".roboclaw/conversations");

    // 获取或创建会话
    auto session = session_manager_->getOrCreateLatestSession();
    session_manager_->setCurrentSession(session);

    while (!should_exit_) {
        showPrompt();
        std::string input = readInput();

        if (input.empty()) {
            continue;
        }

        // 处理命令
        if (input[0] == '/') {
            if (!handleSlashCommand(input)) {
                break;
            }
            continue;
        }

        // 处理普通消息
        processMessage(input);

        // 保存会话
        saveCurrentSession();
    }
}

bool InteractiveMode::processMessage(const std::string& message) {
    if (message.empty()) {
        return false;
    }

    std::cout << "\n";

    // 发送给Agent处理
    AgentResponse response = agent_->process(message);

    // 显示响应
    displayResponse(response);

    return response.success;
}

void InteractiveMode::displayResponse(const AgentResponse& response) {
    using namespace Color;

    if (!response.success) {
        // 使用统一的错误样式盒子
        UI::drawError(response.error);
        return;
    }

    if (!response.content.empty()) {
        // 使用圆角盒子展示助手回复，提升可读性与美观度
        UI::drawBox(
            "Assistant / 助手",
            response.content,
            0,
            UI::BoxStyle::ROUNDED,
            UI::Alignment::LEFT
        );
    }

    if (config_.show_tool_calls && !response.tool_calls.empty()) {
        // 将工具调用信息放入单独盒子中
        std::vector<std::string> toolLines;
        for (const auto& call : response.tool_calls) {
            std::ostringstream oss;
            oss << "[Tool: " << call.name << "] " << call.arguments.dump();
            toolLines.push_back(oss.str());
        }

        UI::drawBox(
            "Tool Calls / 工具调用",
            toolLines,
            0,
            UI::BoxStyle::ASCII,
            UI::Alignment::LEFT
        );
    }

    if (response.total_input_tokens > 0 || response.total_output_tokens > 0) {
        std::ostringstream oss;
        oss << "Tokens: " << response.total_input_tokens
            << " input, " << response.total_output_tokens << " output";
        UI::drawInfo(oss.str());
    }
}

void InteractiveMode::displayToolCall(const ChatMessage::ToolCall& call, const ToolResult& result) {
    using namespace Color;

    std::cout << YELLOW << "[Tool: " << call.name << "]" << RESET << " ";
    std::cout << GRAY << call.arguments.dump() << RESET << std::endl;

    if (result.success) {
        std::cout << GREEN << "Result: " << result.content << RESET << std::endl;
    } else {
        std::cout << RED << "Error: " << result.error_message << RESET << std::endl;
    }
}

bool InteractiveMode::handleSlashCommand(const std::string& command) {
    std::istringstream iss(command);
    std::string cmd;
    iss >> cmd;

    // Remove leading /
    if (!cmd.empty() && cmd[0] == '/') {
        cmd = cmd.substr(1);
    }

    std::string args;
    if (iss) {
        std::getline(iss, args);
    }

    // Trim leading whitespace
    if (!args.empty() && args[0] == ' ') {
        args = args.substr(1);
    }

    if (cmd == "exit" || cmd == "quit") {
        setExitFlag(true);
        return false;
    }

    if (cmd == "help") return cmdHelp();
    if (cmd == "config") return cmdConfig();
    if (cmd == "clear") return cmdClear();
    if (cmd == "model") return cmdModel(args);
    if (cmd == "agent") return cmdAgent(args);
    if (cmd == "browser") return cmdBrowser(args);
    if (cmd == "link") return cmdLink(args);
    if (cmd == "skills") return cmdSkills(args);

    std::cout << Color::RED << "Unknown command: " << cmd << Color::RESET << std::endl;
    std::cout << "Type /help for available commands" << std::endl;
    return true;
}

bool InteractiveMode::cmdHelp() {
    showHelp();
    return true;
}

bool InteractiveMode::cmdConfig() {
    std::string configPath = ConfigManager::getConfigPath();
    using namespace Color;

    std::cout << CYAN << "Configuration file location: " << RESET << configPath << "\n\n";
    std::cout << YELLOW << "To edit, use: " << RESET
              << "nano " << configPath << " or vim " << configPath << "\n";

#ifdef PLATFORM_MACOS
    std::cout << YELLOW << "macOS: " << RESET << "open " << configPath << "\n";
#elif defined(PLATFORM_LINUX)
    std::cout << YELLOW << "Linux: " << RESET << "xdg-open " << configPath << "\n";
#elif defined(PLATFORM_WINDOWS)
    std::cout << YELLOW << "Windows: " << RESET << "notepad " << configPath << "\n";
#endif

    return true;
}

bool InteractiveMode::cmdClear() {
    Terminal::clear();
    showWelcome();
    return true;
}

bool InteractiveMode::cmdAgent(const std::string& args) {
    using namespace Color;

    // TODO: Implement agent management when Agent exposes tool_executor
    if (args.empty() || args == "list") {
        std::cout << YELLOW << "Agent management feature coming soon!" << RESET << std::endl;
        std::cout << "Available commands: list, show <id>, launch <id>" << std::endl;
        return true;
    }

    std::cout << YELLOW << "Agent commands: list, show <id>, launch <id>" << RESET << std::endl;
    return true;
}

bool InteractiveMode::cmdBrowser(const std::string& args) {
    using namespace Color;

    // TODO: Implement browser automation when Agent exposes tool_executor
    if (args.empty()) {
        std::cout << YELLOW << "Browser automation feature coming soon!" << RESET << std::endl;
        std::cout << "Available commands: open, screenshot, navigate <url>" << std::endl;
        return true;
    }

    std::cout << YELLOW << "Browser commands: open, screenshot, navigate <url>" << RESET << std::endl;
    return true;
}

bool InteractiveMode::cmdLink(const std::string& args) {
    using namespace Color;
    using namespace cli;

    LinkCommand linkCmd;

    if (args.empty() || args == "list") {
        // 列出可用平台
        auto platforms = linkCmd.getAvailablePlatforms();
        std::cout << CYAN << "可用社交平台 / Available Platforms:" << RESET << "\n\n";

        for (const auto& platform : platforms) {
            std::cout << "  " << YELLOW << platform.id << RESET << " - "
                      << platform.name << " (" << platform.description << ")\n";
            if (platform.enabled) {
                std::cout << "    " << GREEN << "✓ 已启用 / Enabled" << RESET << "\n";
            } else {
                std::cout << "    " << GRAY << "✗ 未启用 / Disabled" << RESET << "\n";
            }
            std::cout << "\n";
        }

        std::cout << GRAY << "用法 / Usage:\n";
        std::cout << "  /link connect <platform>  连接平台\n";
        std::cout << "  /link disconnect <platform> 断开连接\n";
        std::cout << RESET;
        return true;
    }

    std::istringstream iss(args);
    std::string action;
    iss >> action;

    if (action == "connect") {
        std::string platform;
        iss >> platform;

        if (platform.empty()) {
            std::cout << RED << "请指定平台 / Please specify platform" << RESET << "\n";
            std::cout << "用法: /link connect <platform>" << "\n";
            return true;
        }

        std::cout << YELLOW << "正在连接 / Connecting to " << platform << "..." << RESET << "\n";

        if (platform == "telegram") {
            std::cout << CYAN << "请输入 Telegram Bot Token:" << RESET << "\n";
            std::cout << GRAY << "(从 @BotFather 获取)" << RESET << "\n";
            std::cout << ">>> ";

            std::string token;
            std::getline(std::cin, token);

            nlohmann::json config;
            config["bot_token"] = token;

            if (linkCmd.connectToPlatform(platform, config)) {
                std::cout << GREEN << "✓ 连接成功 / Connected successfully!" << RESET << "\n";
            } else {
                std::cout << RED << "✗ 连接失败 / Connection failed" << RESET << "\n";
            }
        } else {
            std::cout << YELLOW << "平台 " << platform << " 尚未实现 / Not implemented yet" << RESET << "\n";
        }
        return true;
    }

    if (action == "disconnect") {
        std::string platform;
        iss >> platform;

        std::cout << YELLOW << "正在断开 / Disconnecting from " << platform << "..." << RESET << "\n";
        // TODO: 实现断开连接
        std::cout << GRAY << "断开连接功能即将推出 / Disconnect feature coming soon" << RESET << "\n";
        return true;
    }

    std::cout << RED << "未知操作 / Unknown action: " << action << RESET << "\n";
    std::cout << "可用操作 / Available actions: list, connect, disconnect\n";
    return true;
}

bool InteractiveMode::cmdModel(const std::string& args) {
    using namespace Color;

    if (args.empty()) {
        // 显示当前模型信息和帮助
        const auto& config = config_manager_.getConfig();
        std::string currentModel = config.default_config.model;
        std::string currentProvider = ConfigManager::providerToString(config.default_config.provider);

        std::cout << CYAN << "当前模型配置 / Current Model:" << RESET << "\n";
        std::cout << "  模型 / Model:   " << YELLOW << currentModel << RESET << "\n";
        std::cout << "  提供商 / Provider: " << CYAN << currentProvider << RESET << "\n\n";

        std::cout << GRAY << "用法 / Usage:" << RESET << "\n";
        std::cout << "  " << GREEN << "/model list" << RESET << "                     列出所有可用模型 / List all models\n";
        std::cout << "  " << GREEN << "/model switch <model>" << RESET << "            切换模型 / Switch model\n";
        std::cout << "  " << GREEN << "/model add <name> <url> <key>" << RESET << "  添加新模型 / Add new model\n";
        std::cout << "\n";
        std::cout << "示例 / Examples:\n";
        std::cout << "  /model switch claude-3-5-sonnet-20241022\n";
        std::cout << "  /model add gpt-4 https://api.openai.com/v1 sk-xxx\n";
        return true;
    }

    std::istringstream iss(args);
    std::string action;
    iss >> action;

    if (action == "list") {
        // 列出所有已配置的提供商和模型
        const auto& config = config_manager_.getConfig();

        std::cout << CYAN << "已配置的模型 / Configured Models:" << RESET << "\n\n";

        for (const auto& pair : config.providers) {
            const auto& provider = pair.second;
            std::cout << "  [" << CYAN << provider.name << RESET << "] ";
            if (!provider.api_key.empty()) {
                std::cout << GREEN << "✓ 已配置 / Configured" << RESET;
            } else {
                std::cout << RED << "✗ 未配置 / Not configured" << RESET;
            }
            std::cout << "\n";
        }
        std::cout << "\n";
        return true;
    }

    if (action == "switch") {
        std::string modelName;
        iss >> modelName;

        if (modelName.empty()) {
            std::cout << RED << "错误 / Error: 请指定模型名称 / Please specify model name" << RESET << "\n";
            std::cout << "示例 / Example: /model switch claude-3-5-sonnet-20241022\n";
            return true;
        }

        std::cout << YELLOW << "模型切换功能需要配置文件持久化，即将推出！" << RESET << "\n";
        std::cout << "Model switching requires config persistence, coming soon!\n";
        std::cout << "当前会话可使用: /model add 添加新模型配置\n";
        return true;
    }

    if (action == "add") {
        std::string modelName, apiUrl, apiKey;
        iss >> modelName >> apiUrl >> apiKey;

        if (modelName.empty() || apiUrl.empty() || apiKey.empty()) {
            std::cout << RED << "错误 / Error: 参数不完整 / Incomplete parameters" << RESET << "\n";
            std::cout << "格式 / Format: /model add <name> <url> <key>\n";
            std::cout << "示例 / Example: /model add gpt-4 https://api.openai.com/v1 sk-xxx\n";
            return true;
        }

        std::cout << YELLOW << "添加模型功能需要配置文件持久化，即将推出！" << RESET << "\n";
        std::cout << "Add model feature requires config persistence, coming soon!\n";
        std::cout << "临时方案 / Temporary: 请使用 /config 编辑配置文件手动添加\n";
        return true;
    }

    std::cout << RED << "未知命令 / Unknown command: " << action << RESET << "\n";
    std::cout << "使用 /model 查看帮助 / Use /model for help\n";
    return true;
}

bool InteractiveMode::cmdSkills(const std::string& args) {
    using namespace Color;

    if (args.empty() || args == "list") {
        // List all available skills
        std::cout << CYAN << "Available Skills / 可用技能:" << RESET << "\n\n";
        std::cout << GRAY << "Skills directory: .roboclaw/skills/" << RESET << "\n\n";

        std::cout << YELLOW << "Built-in skills / 内置技能:" << RESET << "\n";
        std::cout << "  • " << GREEN << "motion" << RESET << "   - Robot motion control / 机器人运动控制\n";
        std::cout << "  • " << GREEN << "sensor" << RESET << "   - Sensor data reading / 传感器数据读取\n";
        std::cout << "  • " << GREEN << "gripper" << RESET << "  - Gripper control / 夹爪控制\n";

        std::cout << "\n" << YELLOW << "Usage / 用法:" << RESET << "\n";
        std::cout << "  " << GREEN << "/skills list" << RESET << "             List all skills / 列出所有技能\n";
        std::cout << "  " << GREEN << "/skills info <name>" << RESET << "       Show skill details / 显示技能详情\n";
        std::cout << "  " << GREEN << "/<skillname>" << RESET << "              Invoke skill directly / 直接调用技能\n";
        std::cout << "  " << GREEN << "natural trigger" << RESET << "           Use natural language / 使用自然语言\n";

        return true;
    }

    std::istringstream iss(args);
    std::string action;
    iss >> action;

    if (action == "info" || action == "show") {
        std::string skillName;
        iss >> skillName;

        if (skillName.empty()) {
            std::cout << RED << "Please specify skill name / 请指定技能名称" << RESET << "\n";
            return true;
        }

        std::cout << YELLOW << "Skill info / 技能信息: " << skillName << RESET << "\n";
        std::cout << GRAY << "Feature coming soon! / 功能即将推出！" << RESET << "\n";
        return true;
    }

    std::cout << RED << "Unknown action / 未知操作: " << action << RESET << "\n";
    std::cout << "Available: list, info / 可用: list, info\n";
    return true;
}

bool InteractiveMode::handleCommand(const std::string& input) {
    std::istringstream iss(input);
    std::string command;
    iss >> command;

    // 移除前缀
    if (command[0] == '/' || command[0] == '.') {
        command = command.substr(1);
    }

    // 转换为小写
    std::transform(command.begin(), command.end(), command.begin(), ::tolower);

    if (command == "exit" || command == "quit" || command == "q") {
        setExitFlag(true);
        return false;
    }

    if (command == "help" || command == "h" || command == "?") {
        showHelp();
        return true;
    }

    if (command == "clear" || command == "cls") {
        #ifdef PLATFORM_WINDOWS
        system("cls");
        #else
        system("clear");
        #endif
        showBanner();
        return true;
    }

    if (command == "session" || command == "s") {
        showSessionInfo();
        return true;
    }

    if (command == "branch" || command == "b") {
        std::string subCommand;
        if (iss >> subCommand) {
            if (subCommand == "list" || subCommand == "l") {
                listBranches();
            } else if (subCommand == "new" || subCommand == "n") {
                std::string branchName;
                if (iss >> branchName) {
                    createBranch(branchName);
                } else {
                    std::cout << "请指定分支名称" << std::endl;
                }
            } else if (subCommand == "switch" || subCommand == "s") {
                std::string branchName;
                if (iss >> branchName) {
                    switchBranch(branchName);
                } else {
                    std::cout << "请指定分支名称" << std::endl;
                }
            } else {
                std::cout << "未知的分支子命令: " << subCommand << std::endl;
            }
        } else {
            listBranches();
        }
        return true;
    }

    if (command == "save") {
        saveCurrentSession();
        std::cout << "会话已保存" << std::endl;
        return true;
    }

    std::cout << "未知命令: " << command << " (输入 /help 查看帮助)" << std::endl;
    return true;
}

void InteractiveMode::showWelcome() {
    using namespace Color;

    // Clear screen
    Terminal::clear();

    // Draw logo
    UI::drawLogo();

    // Get model info
    const auto& config = config_manager_.getConfig();
    std::string modelName = config.default_config.model;
    std::string providerName = ConfigManager::providerToString(config.default_config.provider);
    UI::drawModelInfo(modelName, providerName);

    // Draw usage tips
    UI::drawUsageTips();
}

void InteractiveMode::showBanner() {
    auto session = session_manager_->getCurrentSession();
    if (!session) {
        return;
    }
    const auto& config = config_manager_.getConfig();
    std::string provider = ConfigManager::providerToString(config.default_config.provider);
    std::string model = config.default_config.model;

    std::string sessionTitle = session->getConversationId().empty()
        ? "Untitled Session"
        : session->getConversationId();

    std::string branchName;
    auto currentNode = session->getCurrentNode();
    if (currentNode && !currentNode->getBranchName().empty()) {
        branchName = currentNode->getBranchName();
    }

    std::ostringstream header;
    header << "RoboClaw Interactive  "
           << "| Provider: " << provider
           << " | Model: " << model
           << " | Session: " << sessionTitle;

    if (!branchName.empty()) {
        header << " | Branch: " << branchName;
    }

    UI::drawRule(header.str());
}

void InteractiveMode::showPrompt() {
    using namespace Color;
    std::cout << CYAN << "> " << RESET;
    std::cout.flush();
}

std::string InteractiveMode::readInput() {
    std::string line;
    std::getline(std::cin, line);
    return line;
}

void InteractiveMode::showHelp() {
    using namespace Color;

    std::string helpContent = R"(
Direct input to start conversation / 直接输入开始对话

Slash Commands / 斜杠命令:
  /help       Show this help / 显示帮助
  /config     Edit configuration / 编辑配置
  /model      Switch or add models / 切换或添加模型
  /skills     List and manage skills / 技能列表和管理
  /clear      Clear conversation / 清空对话
  /link       Connect social platforms / 连接社交平台
  Ctrl+D      Exit / 退出
)";

    std::vector<std::string> contentLines;
    std::istringstream iss(helpContent);
    std::string line;
    while (std::getline(iss, line)) {
        contentLines.push_back(line);
    }

    UI::drawBox("Available Commands / 可用命令", contentLines);
}

void InteractiveMode::showSessionInfo() {
    auto session = session_manager_->getCurrentSession();
    if (!session) {
        std::cout << "当前没有活动会话" << std::endl;
        return;
    }

    std::cout << "\n会话信息:\n";
    std::cout << "  ID: " << session->getConversationId() << "\n";

    auto currentNode = session->getCurrentNode();
    if (currentNode) {
        std::cout << "  当前节点: " << currentNode->getId() << "\n";
        if (!currentNode->getBranchName().empty()) {
            std::cout << "  当前分支: " << currentNode->getBranchName() << "\n";
        }
    }

    auto allNodes = session->getAllNodes();
    std::cout << "  消息数: " << allNodes.size() << "\n\n";
}

void InteractiveMode::createBranch(const std::string& branchName) {
    auto session = session_manager_->getCurrentSession();
    if (!session) {
        std::cout << "当前没有活动会话" << std::endl;
        return;
    }

    auto currentNode = session->getCurrentNode();
    if (!currentNode) {
        std::cout << "无法获取当前节点" << std::endl;
        return;
    }

    auto branch = session->createBranch(currentNode->getId(), branchName);
    if (branch) {
        session->switchToNode(branch->getId());
        session_manager_->saveSession(session);
        std::cout << "已创建分支: " << branchName << std::endl;
        showBanner();
    } else {
        std::cout << "创建分支失败" << std::endl;
    }
}

void InteractiveMode::switchBranch(const std::string& branchName) {
    auto session = session_manager_->getCurrentSession();
    if (!session) {
        std::cout << "当前没有活动会话" << std::endl;
        return;
    }

    // 查找分支
    auto allNodes = session->getAllNodes();
    for (const auto& node : allNodes) {
        if (node->getBranchName() == branchName) {
            session->switchToNode(node->getId());
            session_manager_->saveSession(session);
            std::cout << "已切换到分支: " << branchName << std::endl;
            showBanner();
            return;
        }
    }

    std::cout << "未找到分支: " << branchName << std::endl;
}

void InteractiveMode::listBranches() {
    auto session = session_manager_->getCurrentSession();
    if (!session) {
        std::cout << "当前没有活动会话" << std::endl;
        return;
    }

    auto branches = session->getBranchNames();
    if (branches.empty()) {
        std::cout << "当前没有分支" << std::endl;
        return;
    }

    std::cout << "\n分支列表:\n";
    for (const auto& branch : branches) {
        std::cout << "  - " << branch << "\n";
    }
    std::cout << std::endl;
}

void InteractiveMode::saveCurrentSession() {
    auto session = session_manager_->getCurrentSession();
    if (session) {
        session_manager_->saveSession(session);
    }
}

} // namespace roboclaw
