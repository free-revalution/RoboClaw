// BashTool实现

#include "bash_tool.h"
#include <sstream>
#include <algorithm>
#include <regex>

#ifdef PLATFORM_UNIX
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#endif

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <shellapi.h>
#endif

namespace roboclaw {

BashTool::BashTool()
    : ToolBase("bash", "执行shell命令")
    , default_timeout_(30)  // 默认30秒超时
{
    // 默认禁止的危险命令
    forbidden_commands_ = {
        "rm -rf /",
        "rm -rf /*",
        "rm -rf \\",
        "mkfs",
        "dd if=/dev/zero",
        "chmod -R 777 /",
        "chown -R",
        ":(){ :|:& };:",  // fork bomb
        "rm -rf ~",
        "rm -rf /home",
        "rm -rf /usr",
        "rm -rf /etc",
        "rm -rf /bin",
        "rm -rf /sbin",
        "rm -rf /var",
        "rm -rf /opt"
    };
}

ToolDescription BashTool::getDescription() const {
    ToolDescription desc;
    desc.name = name_;
    desc.description = description_;

    desc.parameters = {
        {"command", "string", "要执行的命令（必需）", true, ""},
        {"timeout", "integer", "超时时间，秒（可选，默认30秒）", false, "30"}
    };

    return desc;
}

bool BashTool::validateParams(const json& params) const {
    if (!hasRequiredParam(params, "command")) {
        return false;
    }

    std::string command = getStringParam(params, "command");
    if (command.empty()) {
        return false;
    }

    // 检查是否是被禁止的命令
    if (isCommandForbidden(command)) {
        return false;
    }

    int timeout = getIntParam(params, "timeout", default_timeout_);
    if (timeout <= 0 || timeout > 300) {  // 最多5分钟
        return false;
    }

    return true;
}

ToolResult BashTool::execute(const json& params) {
    if (!validateParams(params)) {
        return ToolResult::error("参数验证失败：command是必需参数，或命令被禁止");
    }

    std::string command = getStringParam(params, "command");
    int timeout = getIntParam(params, "timeout", default_timeout_);

    LOG_DEBUG("执行命令: " + command + " (timeout=" + std::to_string(timeout) + "s)");

    std::string stdout, stderr, error;
    int exitCode;

    if (!executeCommand(command, timeout, stdout, stderr, exitCode, error)) {
        return ToolResult::error(error);
    }

    // 构建元数据
    json metadata;
    metadata["command"] = command;
    metadata["exit_code"] = exitCode;
    metadata["timeout"] = timeout;
    metadata["timed_out"] = (exitCode == -1);

    // 构建输出内容
    std::stringstream content;
    if (!stdout.empty()) {
        content << "标准输出:\n" << stdout << "\n";
    }
    if (!stderr.empty()) {
        content << "标准错误:\n" << stderr << "\n";
    }

    LOG_DEBUG("命令执行完成: exit_code=" + std::to_string(exitCode));

    return ToolResult::ok(content.str(), metadata);
}

bool BashTool::isCommandForbidden(const std::string& command) const {
    // 将命令转换为小写进行比较
    std::string lowerCommand = command;
    std::transform(lowerCommand.begin(), lowerCommand.end(),
                   lowerCommand.begin(), ::tolower);

    // 移除多余空格
    lowerCommand = std::regex_replace(lowerCommand, std::regex("\\s+"), " ");
    lowerCommand = std::regex_replace(lowerCommand, std::regex("^\\s+|\\s+$"), "");

    for (const auto& forbidden : forbidden_commands_) {
        std::string lowerForbidden = forbidden;
        std::transform(lowerForbidden.begin(), lowerForbidden.end(),
                       lowerForbidden.begin(), ::tolower);

        if (lowerCommand.find(lowerForbidden) != std::string::npos) {
            LOG_WARNING("禁止的命令: " + command);
            return true;
        }
    }

    return false;
}

#ifdef PLATFORM_UNIX
bool BashTool::executeCommandUnix(const std::string& command, int timeout,
                                  std::string& stdout, std::string& stderr,
                                  int& exitCode, std::string& error) {
    int stdout_pipe[2], stderr_pipe[2];
    if (pipe(stdout_pipe) == -1 || pipe(stderr_pipe) == -1) {
        error = "无法创建管道";
        return false;
    }

    pid_t pid = fork();
    if (pid == -1) {
        error = "无法创建子进程";
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        close(stderr_pipe[0]);
        close(stderr_pipe[1]);
        return false;
    }

    if (pid == 0) {  // 子进程
        // 设置进程组
        setpgid(0, 0);

        // 重定向标准输出和标准错误
        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);

        // 关闭不需要的管道端
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        close(stderr_pipe[0]);
        close(stderr_pipe[1]);

        // 执行命令
        execl("/bin/sh", "sh", "-c", command.c_str(), nullptr);
        _exit(127);  // 执行失败
    }

    // 父进程
    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    // 设置超时
    alarm(timeout);

    // 读取输出
    char buffer[4096];
    ssize_t count;

    while ((count = read(stdout_pipe[0], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[count] = '\0';
        stdout += buffer;
    }
    close(stdout_pipe[0]);

    while ((count = read(stderr_pipe[0], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[count] = '\0';
        stderr += buffer;
    }
    close(stderr_pipe[0]);

    // 等待子进程结束
    int status;
    pid_t result = waitpid(pid, &status, 0);

    // 取消超时
    alarm(0);

    if (result == -1) {
        if (errno == EINTR) {
            // 超时，杀死进程组
            kill(-pid, SIGKILL);
            waitpid(pid, &status, 0);
            exitCode = -1;  // 超时标记
            error = "命令执行超时";
            return false;
        }
        error = "等待子进程失败";
        return false;
    }

    if (WIFEXITED(status)) {
        exitCode = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        exitCode = 128 + WTERMSIG(status);
    } else {
        exitCode = -1;
    }

    return true;
}
#endif

#ifdef PLATFORM_WINDOWS
bool BashTool::executeCommandWindows(const std::string& command, int timeout,
                                    std::string& stdout, std::string& stderr,
                                    int& exitCode, std::string& error) {
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    HANDLE stdout_read, stdout_write;
    HANDLE stderr_read, stderr_write;

    if (!CreatePipe(&stdout_read, &stdout_write, &sa, 0) ||
        !CreatePipe(&stderr_read, &stderr_write, &sa, 0)) {
        error = "无法创建管道";
        return false;
    }

    // 设置不继承读端
    SetHandleInformation(stdout_read, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(stderr_read, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = stdout_write;
    si.hStdError = stderr_write;
    ZeroMemory(&pi, sizeof(pi));

    // 构建完整命令
    std::string fullCommand = "cmd /c " + command;

    if (!CreateProcessA(nullptr, const_cast<char*>(fullCommand.c_str()),
                       nullptr, nullptr, TRUE, CREATE_NO_WINDOW,
                       nullptr, nullptr, &si, &pi)) {
        error = "无法创建进程";
        CloseHandle(stdout_read);
        CloseHandle(stdout_write);
        CloseHandle(stderr_read);
        CloseHandle(stderr_write);
        return false;
    }

    CloseHandle(stdout_write);
    CloseHandle(stderr_write);

    // 等待进程结束（带超时）
    DWORD waitResult = WaitForSingleObject(pi.hProcess, timeout * 1000);

    if (waitResult == WAIT_TIMEOUT) {
        TerminateProcess(pi.hProcess, 1);
        exitCode = -1;
        error = "命令执行超时";
        CloseHandle(stdout_read);
        CloseHandle(stderr_read);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return false;
    }

    // 读取输出
    char buffer[4096];
    DWORD bytesRead;

    while (ReadFile(stdout_read, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        stdout += buffer;
    }
    CloseHandle(stdout_read);

    while (ReadFile(stderr_read, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        stderr += buffer;
    }
    CloseHandle(stderr_read);

    // 获取退出码
    GetExitCodeProcess(pi.hProcess, (DWORD*)&exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return true;
}
#endif

bool BashTool::executeCommand(const std::string& command, int timeout,
                              std::string& stdout, std::string& stderr,
                              int& exitCode, std::string& error) {
    #ifdef PLATFORM_UNIX
        return executeCommandUnix(command, timeout, stdout, stderr, exitCode, error);
    #elif defined(PLATFORM_WINDOWS)
        return executeCommandWindows(command, timeout, stdout, stderr, exitCode, error);
    #else
        error = "不支持的平台";
        return false;
    #endif
}

} // namespace roboclaw
