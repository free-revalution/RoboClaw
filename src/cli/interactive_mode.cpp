// InteractiveMode实现

#include "interactive_mode.h"
#include "../utils/logger.h"
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
        if (input[0] == '/' || input[0] == '.') {
            if (!handleCommand(input)) {
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
    if (!response.success) {
        std::cout << "\033[31m";  // 红色
        std::cout << "错误: " << response.error;
        std::cout << "\033[0m" << std::endl;
        return;
    }

    // 显示内容
    if (!response.content.empty()) {
        std::cout << response.content << std::endl;
    }

    // 显示工具调用
    if (config_.show_tool_calls && !response.tool_calls.empty()) {
        std::cout << "\n\033[36m";  // 青色
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━";
        std::cout << "\033[0m" << std::endl;

        for (const auto& call : response.tool_calls) {
            std::cout << "\033[33m[Tool: " << call.name << "]\033[0m ";
            std::cout << call.arguments.dump() << std::endl;
        }

        std::cout << "\033[36m";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━";
        std::cout << "\033[0m\n" << std::endl;
    }

    // 显示token使用
    if (response.total_input_tokens > 0 || response.total_output_tokens > 0) {
        std::cout << "\033[90m";  // 灰色
        std::cout << "Tokens: " << response.total_input_tokens
                  << " input, " << response.total_output_tokens << " output";
        std::cout << "\033[0m" << std::endl;
    }
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
    std::cout << "\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  欢迎使用 RoboClaw 交互式对话模式\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "\n输入 /help 查看可用命令\n\n";
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
    std::cout << "\033[32m你\033[0m: ";
    std::cout.flush();
}

std::string InteractiveMode::readInput() {
    std::string line;
    std::getline(std::cin, line);
    return line;
}

void InteractiveMode::showHelp() {
    std::cout << "\n可用命令:\n\n";
    std::cout << "  /help, /h, /?     显示此帮助\n";
    std::cout << "  /exit, /quit, /q  退出程序\n";
    std::cout << "  /clear, /cls     清屏\n";
    std::cout << "  /session, /s      显示会话信息\n";
    std::cout << "  /branch, /b       分支管理\n";
    std::cout << "    /branch list    列出所有分支\n";
    std::cout << "    /branch new <name>  创建新分支\n";
    std::cout << "    /branch switch <name> 切换分支\n";
    std::cout << "  /save            保存当前会话\n\n";
    std::cout << "直接输入消息开始对话\n\n";
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
