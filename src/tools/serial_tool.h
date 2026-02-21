// 串口工具 - Serial Port Tool
// 用于嵌入式开发的串口通信工具 / Serial port communication tool for embedded development

#ifndef ROBOCLAW_TOOLS_SERIAL_TOOL_H
#define ROBOCLAW_TOOLS_SERIAL_TOOL_H

#include "tool_base.h"
#include <string>
#include <vector>
#include <memory>

// 平台检测
#if defined(PLATFORM_MACOS) || defined(PLATFORM_LINUX)
#define PLATFORM_UNIX
#endif

namespace roboclaw {

// 串口配置
struct SerialConfig {
    int baud_rate = 115200;       // 波特率 / Baud rate
    int data_bits = 8;            // 数据位 / Data bits
    int stop_bits = 1;            // 停止位 / Stop bits
    char parity = 'N';            // 校验位: 'N'无, 'O'奇, 'E'偶 / Parity: 'N'one, 'O'dd, 'E'ven
    int timeout_ms = 1000;        // 超时时间(毫秒) / Timeout in milliseconds
    bool rts_cts = false;         // 硬件流控 / Hardware flow control
    bool xon_xoff = false;        // 软件流控 / Software flow control
};

// 串口操作类型
enum class SerialAction {
    LIST,      // 列出可用串口 / List available ports
    OPEN,      // 打开串口 / Open port
    CLOSE,     // 关闭串口 / Close port
    READ,      // 读取数据 / Read data
    WRITE,     // 写入数据 / Write data
    CONFIG     // 配置串口 / Configure port
};

// 串口句柄（跨平台）
class SerialHandle {
public:
    virtual ~SerialHandle() = default;
    virtual bool isOpen() const = 0;
    virtual bool read(std::string& data, size_t max_size) = 0;
    virtual bool write(const std::string& data) = 0;
    virtual bool configure(const SerialConfig& config) = 0;
    virtual void close() = 0;

    // 获取端口名称
    std::string getPortName() const { return port_name_; }

protected:
    std::string port_name_;
};

// 串口工具类
class SerialTool : public ToolBase {
public:
    SerialTool();
    ~SerialTool() override;

    // 获取工具描述
    ToolDescription getToolDescription() const override;

    // 验证参数
    bool validateParams(const json& params) const override;

    // 执行工具
    ToolResult execute(const json& params) override;

    // 列出可用串口
    ToolResult listPorts();

    // 打开串口
    ToolResult openPort(const std::string& port, const SerialConfig& config);

    // 关闭串口
    ToolResult closePort(const std::string& port);

    // 读取数据
    ToolResult readData(const std::string& port, size_t max_size);

    // 写入数据
    ToolResult writeData(const std::string& port, const std::string& data);

    // 配置串口
    ToolResult configurePort(const std::string& port, const SerialConfig& config);

private:
    // 解析串口配置
    SerialConfig parseConfig(const json& params) const;

    // 打开串口（平台相关）
    #ifdef PLATFORM_UNIX
    std::unique_ptr<SerialHandle> openPortUnix(const std::string& port, const SerialConfig& config);
    #endif
    #ifdef PLATFORM_WINDOWS
    std::unique_ptr<SerialHandle> openPortWindows(const std::string& port, const SerialConfig& config);
    #endif

    // 查找可用串口（平台相关）
    #ifdef PLATFORM_UNIX
    std::vector<std::string> findPortsUnix();
    #endif
    #ifdef PLATFORM_WINDOWS
    std::vector<std::string> findPortsWindows();
    #endif

    // 已打开的串口映射
    std::map<std::string, std::unique_ptr<SerialHandle>> open_ports_;

    // 读写锁保证线程安全
    mutable std::shared_mutex ports_mutex_;
};

} // namespace roboclaw

#endif // ROBOCLAW_TOOLS_SERIAL_TOOL_H
