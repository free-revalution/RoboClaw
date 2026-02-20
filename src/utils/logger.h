// 日志系统 - Logger
// 提供分级日志记录功能，支持控制台和文件输出

#ifndef ROBOCLAW_UTILS_LOGGER_H
#define ROBOCLAW_UTILS_LOGGER_H

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>

namespace roboclaw {

// 日志级别枚举
enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3
};

// Logger类 - 单例模式
class Logger {
public:
    // 获取单例实例
    static Logger& getInstance();

    // 禁止拷贝和移动
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    // 设置日志级别
    void setLogLevel(LogLevel level);
    LogLevel getLogLevel() const;

    // 设置日志文件路径
    void setLogFile(const std::string& filepath);

    // 启用/禁用控制台输出
    void setConsoleOutput(bool enabled);

    // 启用/禁用文件输出
    void setFileOutput(bool enabled);

    // 记录日志（主要方法）
    void log(LogLevel level, const std::string& message, const std::string& file = "", int line = 0);

    // 便捷方法
    void debug(const std::string& message, const std::string& file = "", int line = 0);
    void info(const std::string& message, const std::string& file = "", int line = 0);
    void warning(const std::string& message, const std::string& file = "", int line = 0);
    void error(const std::string& message, const std::string& file = "", int line = 0);

private:
    Logger();  // 私有构造函数
    ~Logger();

    // 获取当前时间字符串
    std::string getCurrentTime() const;

    // 获取级别名称
    std::string getLevelName(LogLevel level) const;

    // 获取级别颜色（ANSI转义码）
    std::string getLevelColor(LogLevel level) const;

    // 格式化日志消息
    std::string formatMessage(LogLevel level, const std::string& message, const std::string& file, int line) const;

    // 写入日志
    void writeLog(const std::string& formattedMessage);

    LogLevel min_level_;
    bool console_output_;
    bool file_output_;
    std::ofstream log_file_;
    std::mutex mutex_;  // 线程安全
};

// 便捷宏定义
#define LOG_DEBUG(msg) \
    roboclaw::Logger::getInstance().debug(msg, __FILE__, __LINE__)
#define LOG_INFO(msg) \
    roboclaw::Logger::getInstance().info(msg, __FILE__, __LINE__)
#define LOG_WARNING(msg) \
    roboclaw::Logger::getInstance().warning(msg, __FILE__, __LINE__)
#define LOG_ERROR(msg) \
    roboclaw::Logger::getInstance().error(msg, __FILE__, __LINE__)

} // namespace roboclaw

#endif // ROBOCLAW_UTILS_LOGGER_H
