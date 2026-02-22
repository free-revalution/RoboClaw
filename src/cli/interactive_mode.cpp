// InteractiveMode实现

#include "interactive_mode.h"
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
        std::cout << RED << "Error: " << response.error << RESET << std::endl;
        return;
    }

    if (!response.content.empty()) {
        std::cout << WHITE << response.content << RESET << std::endl;
    }

    if (config_.show_tool_calls && !response.tool_calls.empty()) {
        std::cout << GRAY;
        UI::drawSeparator("single");
        std::cout << RESET;

        for (const auto& call : response.tool_calls) {
            std::cout << YELLOW << "[Tool: " << call.name << "]" << RESET << " ";
            std::cout << GRAY << call.arguments.dump() << RESET << std::endl;
        }

        std::cout << GRAY;
        UI::drawSeparator("single");
        std::cout << RESET << std::endl;
    }

    if (response.total_input_tokens > 0 || response.total_output_tokens > 0) {
        std::cout << GRAY << "Tokens: " << response.total_input_tokens
                  << " input, " << response.total_output_tokens << " output"
                  << RESET << std::endl;
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
    if (cmd == "agent") return cmdAgent(args);
    if (cmd == "browser") return cmdBrowser(args);

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

    // 获取终端宽度
    #ifdef PLATFORM_WINDOWS
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    #else
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int width = w.ws_col;
    #endif

    if (width > 80) width = 80;

    std::string separator(width - 1, '-');

    std::cout << "\033[90m" << separator << "\033[0m" << std::endl;
    std::cout << "\033[90m  \033[0m";
    std::cout << "RoboClaw v0.1.0";
    std::cout << " | " << provider << " " << model;

    auto currentSession = session_manager_->getCurrentSession();
    if (currentSession) {
        auto currentNode = currentSession->getCurrentNode();
        if (currentNode && !currentNode->getBranchName().empty()) {
            std::cout << " | 分支: \033[33m" << currentNode->getBranchName() << "\033[0m";
        }
    }

    std::cout << "\n\033[90m" << separator << "\033[0m" << std::endl;
}

void InteractiveMode::showPrompt() {
    using namespace Color;
    std::cout << CYAN << ">>> " << RESET;
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
  /clear      Clear conversation / 清空对话
  /agent      Manage AI Agents / 管理 AI Agents
  /browser    Browser automation / 浏览器自动化
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
