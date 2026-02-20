// Logger实现

#include "logger.h"
#include <filesystem>

namespace roboclaw {

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger()
    : min_level_(LogLevel::INFO)
    , console_output_(true)
    , file_output_(false)
{
}

Logger::~Logger() {
    if (log_file_.is_open()) {
        log_file_.close();
    }
}

void Logger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    min_level_ = level;
}

LogLevel Logger::getLogLevel() const {
    return min_level_;
}

void Logger::setLogFile(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(mutex_);

    // 关闭旧文件
    if (log_file_.is_open()) {
        log_file_.close();
    }

    // 创建目录（如果不存在）
    std::filesystem::path logPath(filepath);
    if (logPath.has_parent_path()) {
        std::filesystem::create_directories(logPath.parent_path());
    }

    // 打开新文件
    log_file_.open(filepath, std::ios::app);
    if (!log_file_.is_open()) {
        std::cerr << "无法打开日志文件: " << filepath << std::endl;
    }
}

void Logger::setConsoleOutput(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    console_output_ = enabled;
}

void Logger::setFileOutput(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    file_output_ = enabled;
}

std::string Logger::getCurrentTime() const {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

std::string Logger::getLevelName(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARN";
        case LogLevel::ERROR:   return "ERROR";
        default:                return "UNKNOWN";
    }
}

std::string Logger::getLevelColor(LogLevel level) const {
    // ANSI颜色代码
    #ifdef PLATFORM_WINDOWS
        return "";  // Windows控制台可能不支持ANSI颜色
    #else
        switch (level) {
            case LogLevel::DEBUG:   return "\033[36m";  // 青色
            case LogLevel::INFO:    return "\033[32m";  // 绿色
            case LogLevel::WARNING: return "\033[33m";  // 黄色
            case LogLevel::ERROR:   return "\033[31m";  // 红色
            default:                return "\033[0m";   // 重置
        }
    #endif
}

std::string Logger::formatMessage(LogLevel level, const std::string& message,
                                   const std::string& file, int line) const {
    std::stringstream ss;

    // 时间戳
    ss << "[" << getCurrentTime() << "] ";

    // 日志级别
    #ifndef PLATFORM_WINDOWS
        ss << getLevelColor(level);
    #endif
    ss << "[" << getLevelName(level) << "]";
    #ifndef PLATFORM_WINDOWS
        ss << "\033[0m";  // 重置颜色
    #endif

    // 文件位置（如果有）
    if (!file.empty()) {
        // 只显示文件名，不显示完整路径
        std::filesystem::path filePath(file);
        ss << " [" << filePath.filename().string();
        if (line > 0) {
            ss << ":" << line;
        }
        ss << "]";
    }

    // 消息内容
    ss << " " << message;

    return ss.str();
}

void Logger::writeLog(const std::string& formattedMessage) {
    // 写入控制台
    if (console_output_) {
        std::cout << formattedMessage << std::endl;
    }

    // 写入文件
    if (file_output_ && log_file_.is_open()) {
        // 文件输出不包含ANSI颜色代码
        std::string cleanMessage = formattedMessage;
        #ifndef PLATFORM_WINDOWS
            // 移除ANSI颜色代码
            size_t pos = 0;
            while ((pos = cleanMessage.find("\033[", pos)) != std::string::npos) {
                size_t end = cleanMessage.find('m', pos);
                if (end != std::string::npos) {
                    cleanMessage.erase(pos, end - pos + 1);
                } else {
                    break;
                }
            }
        #endif
        log_file_ << cleanMessage << std::endl;
        log_file_.flush();  // 立即刷新，确保日志写入
    }
}

void Logger::log(LogLevel level, const std::string& message,
                 const std::string& file, int line) {
    if (level < min_level_) {
        return;  // 低于最小级别，不记录
    }

    std::lock_guard<std::mutex> lock(mutex_);
    std::string formatted = formatMessage(level, message, file, line);
    writeLog(formatted);
}

void Logger::debug(const std::string& message, const std::string& file, int line) {
    log(LogLevel::DEBUG, message, file, line);
}

void Logger::info(const std::string& message, const std::string& file, int line) {
    log(LogLevel::INFO, message, file, line);
}

void Logger::warning(const std::string& message, const std::string& file, int line) {
    log(LogLevel::WARNING, message, file, line);
}

void Logger::error(const std::string& message, const std::string& file, int line) {
    log(LogLevel::ERROR, message, file, line);
}

} // namespace roboclaw
