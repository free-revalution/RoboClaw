// 串口工具实现 / Serial Port Tool Implementation

#include "serial_tool.h"
#include "../utils/logger.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

#ifdef PLATFORM_UNIX
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <cstring>
#endif

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <vector>
#endif

namespace roboclaw {

// ============================================================
// Unix/Linux/macOS 串口实现
// ============================================================

#ifdef PLATFORM_UNIX

class UnixSerialHandle : public SerialHandle {
public:
    UnixSerialHandle(int fd, const std::string& port_name)
        : fd_(fd), is_open_(true) {
        port_name_ = port_name;
    }

    ~UnixSerialHandle() override {
        close();
    }

    bool isOpen() const override {
        return is_open_;
    }

    bool read(std::string& data, size_t max_size) override {
        if (!is_open_) return false;

        std::vector<char> buffer(max_size);
        ssize_t bytes_read = ::read(fd_, buffer.data(), max_size);

        if (bytes_read < 0) {
            LOG_ERROR("串口读取失败: " + std::string(strerror(errno)));
            return false;
        }

        data.assign(buffer.data(), bytes_read);
        return true;
    }

    bool write(const std::string& data) override {
        if (!is_open_) return false;

        ssize_t bytes_written = ::write(fd_, data.data(), data.size());

        if (bytes_written < 0) {
            LOG_ERROR("串口写入失败: " + std::string(strerror(errno)));
            return false;
        }

        if (static_cast<size_t>(bytes_written) != data.size()) {
            LOG_WARNING("串口写入不完整");
        }

        return true;
    }

    bool configure(const SerialConfig& config) override {
        if (!is_open_) return false;

        struct termios options;
        if (tcgetattr(fd_, &options) != 0) {
            LOG_ERROR("获取串口属性失败");
            return false;
        }

        // 设置波特率
        speed_t baud;
        switch (config.baud_rate) {
            case 9600:   baud = B9600; break;
            case 19200:  baud = B19200; break;
            case 38400:  baud = B38400; break;
            case 57600:  baud = B57600; break;
            case 115200: baud = B115200; break;
#ifdef B230400
            case 230400: baud = B230400; break;
#endif
#ifdef B460800
            case 460800: baud = B460800; break;
#endif
#ifdef B921600
            case 921600: baud = B921600; break;
#endif
            default:
                LOG_WARNING("不支持的波特率，使用115200");
                baud = B115200;
        }

        cfsetispeed(&options, baud);
        cfsetospeed(&options, baud);

        // 设置数据位
        options.c_cflag &= ~CSIZE;
        switch (config.data_bits) {
            case 5: options.c_cflag |= CS5; break;
            case 6: options.c_cflag |= CS6; break;
            case 7: options.c_cflag |= CS7; break;
            case 8: options.c_cflag |= CS8; break;
            default:
                LOG_WARNING("不支持的数据位，使用8");
                options.c_cflag |= CS8;
        }

        // 设置停止位
        if (config.stop_bits == 2) {
            options.c_cflag |= CSTOPB;
        } else {
            options.c_cflag &= ~CSTOPB;
        }

        // 设置校验位
        options.c_cflag &= ~(PARENB | PARODD);
        if (config.parity == 'O') {
            options.c_cflag |= PARENB | PARODD;
        } else if (config.parity == 'E') {
            options.c_cflag |= PARENB;
        }

        // 设置控制模式
        options.c_cflag |= CLOCAL | CREAD;

        // 设置输入模式
        options.c_iflag &= ~(IXON | IXOFF | IXANY | IGNBRK | INLCR | ICRNL);

        // 设置输出模式
        options.c_oflag &= ~OPOST;

        // 设置本地模式
        options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

        // 设置超时
        options.c_cc[VMIN] = 0;
        options.c_cc[VTIME] = config.timeout_ms / 100;

        // 应用配置
        if (tcsetattr(fd_, TCSANOW, &options) != 0) {
            LOG_ERROR("设置串口属性失败");
            return false;
        }

        tcflush(fd_, TCIOFLUSH);
        return true;
    }

    void close() override {
        if (is_open_) {
            ::close(fd_);
            is_open_ = false;
        }
    }

private:
    int fd_;
    bool is_open_;
};

#endif // PLATFORM_UNIX

// ============================================================
// Windows 串口实现
// ============================================================

#ifdef PLATFORM_WINDOWS

class WindowsSerialHandle : public SerialHandle {
public:
    WindowsSerialHandle(HANDLE handle, const std::string& port_name)
        : handle_(handle), is_open_(true) {
        port_name_ = port_name;
    }

    ~WindowsSerialHandle() override {
        close();
    }

    bool isOpen() const override {
        return is_open_ && handle_ != INVALID_HANDLE_VALUE;
    }

    bool read(std::string& data, size_t max_size) override {
        if (!isOpen()) return false;

        std::vector<char> buffer(max_size);
        DWORD bytes_read = 0;

        if (!ReadFile(handle_, buffer.data(), static_cast<DWORD>(max_size),
                      &bytes_read, NULL)) {
            LOG_ERROR("串口读取失败");
            return false;
        }

        data.assign(buffer.data(), bytes_read);
        return true;
    }

    bool write(const std::string& data) override {
        if (!isOpen()) return false;

        DWORD bytes_written = 0;

        if (!WriteFile(handle_, data.data(), static_cast<DWORD>(data.size()),
                       &bytes_written, NULL)) {
            LOG_ERROR("串口写入失败");
            return false;
        }

        return bytes_written == data.size();
    }

    bool configure(const SerialConfig& config) override {
        if (!isOpen()) return false;

        DCB dcb;
        if (!GetCommState(handle_, &dcb)) {
            LOG_ERROR("获取串口配置失败");
            return false;
        }

        dcb.BaudRate = config.baud_rate;
        dcb.ByteSize = config.data_bits;
        dcb.StopBits = (config.stop_bits == 2) ? TWOSTOPBITS : ONESTOPBIT;

        switch (config.parity) {
            case 'O': dcb.Parity = ODDPARITY; break;
            case 'E': dcb.Parity = EVENPARITY; break;
            default:  dcb.Parity = NOPARITY; break;
        }

        dcb.fOutxCtsFlow = config.rts_cts;
        dcb.fRtsControl = config.rts_cts ? RTS_CONTROL_HANDSHAKE : RTS_CONTROL_ENABLE;
        dcb.fOutX = config.xon_xoff;
        dcb.fInX = config.xon_xoff;

        if (!SetCommState(handle_, &dcb)) {
            LOG_ERROR("设置串口配置失败");
            return false;
        }

        COMMTIMEOUTS timeouts;
        timeouts.ReadIntervalTimeout = config.timeout_ms;
        timeouts.ReadTotalTimeoutMultiplier = 0;
        timeouts.ReadTotalTimeoutConstant = config.timeout_ms;
        timeouts.WriteTotalTimeoutMultiplier = 0;
        timeouts.WriteTotalTimeoutConstant = config.timeout_ms;

        if (!SetCommTimeouts(handle_, &timeouts)) {
            LOG_ERROR("设置串口超时失败");
            return false;
        }

        return true;
    }

    void close() override {
        if (is_open_) {
            CloseHandle(handle_);
            is_open_ = false;
        }
    }

private:
    HANDLE handle_;
    bool is_open_;
};

#endif // PLATFORM_WINDOWS

// ============================================================
// SerialTool 实现
// ============================================================

SerialTool::SerialTool()
    : ToolBase("serial", "串口通信工具 / Serial port communication tool for embedded development") {
}

SerialTool::~SerialTool() {
    std::unique_lock<std::shared_mutex> lock(ports_mutex_);
    open_ports_.clear();
}

ToolDescription SerialTool::getToolDescription() const {
    ToolDescription desc;
    desc.name = "serial";
    desc.description = "串口通信工具 / Serial port communication tool for embedded development. "
                       "Supports list, open, close, read, write, and configure operations.";
    desc.parameters = {
        {"action", "string", "操作类型 / Action: list, open, close, read, write, config", true, ""},
        {"port", "string", "串口名称 / Port name (e.g., /dev/ttyUSB0, COM1)", false, ""},
        {"baud_rate", "integer", "波特率 / Baud rate (default: 115200)", false, "115200"},
        {"data_bits", "integer", "数据位 / Data bits: 5-9 (default: 8)", false, "8"},
        {"stop_bits", "integer", "停止位 / Stop bits: 1-2 (default: 1)", false, "1"},
        {"parity", "string", "校验位 / Parity: N/O/E (default: N)", false, "N"},
        {"timeout", "integer", "超时毫秒 / Timeout in ms (default: 1000)", false, "1000"},
        {"data", "string", "写入数据 / Data to write (for write action)", false, ""},
        {"max_size", "integer", "最大读取字节数 / Max bytes to read (default: 4096)", false, "4096"}
    };
    return desc;
}

bool SerialTool::validateParams(const json& params) const {
    if (!hasRequiredParam(params, "action")) {
        return false;
    }

    std::string action_str = getStringParam(params, "action");
    std::vector<std::string> valid_actions = {"list", "open", "close", "read", "write", "config"};

    if (std::find(valid_actions.begin(), valid_actions.end(), action_str) == valid_actions.end()) {
        return false;
    }

    // 非list操作需要port参数
    if (action_str != "list" && !hasRequiredParam(params, "port")) {
        return false;
    }

    // write操作需要data参数
    if (action_str == "write" && !hasRequiredParam(params, "data")) {
        return false;
    }

    return true;
}

ToolResult SerialTool::execute(const json& params) {
    if (!validateParams(params)) {
        return ToolResult::error("Invalid parameters");
    }

    std::string action_str = getStringParam(params, "action");

    if (action_str == "list") {
        return listPorts();
    }

    std::string port = getStringParam(params, "port");

    if (action_str == "open") {
        SerialConfig config = parseConfig(params);
        return openPort(port, config);
    } else if (action_str == "close") {
        return closePort(port);
    } else if (action_str == "read") {
        size_t max_size = static_cast<size_t>(getIntParam(params, "max_size", 4096));
        return readData(port, max_size);
    } else if (action_str == "write") {
        std::string data = getStringParam(params, "data");
        return writeData(port, data);
    } else if (action_str == "config") {
        SerialConfig config = parseConfig(params);
        return configurePort(port, config);
    }

    return ToolResult::error("Unknown action: " + action_str);
}

SerialConfig SerialTool::parseConfig(const json& params) const {
    SerialConfig config;
    config.baud_rate = getIntParam(params, "baud_rate", 115200);
    config.data_bits = getIntParam(params, "data_bits", 8);
    config.stop_bits = getIntParam(params, "stop_bits", 1);

    std::string parity_str = getStringParam(params, "parity", "N");
    if (!parity_str.empty()) {
        config.parity = std::toupper(parity_str[0]);
    }

    config.timeout_ms = getIntParam(params, "timeout", 1000);
    return config;
}

// ============================================================
// 平台相关实现
// ============================================================

#ifdef PLATFORM_UNIX

std::vector<std::string> SerialTool::findPortsUnix() {
    std::vector<std::string> ports;

    // 搜索 /dev 目录下的串口设备
    const std::vector<std::string> prefixes = {
        "ttyUSB", "ttyACM", "tty.usbserial", "tty.usbmodem",
        "ttyS", "ttyAMA", "ttyO"
    };

    DIR* dir = opendir("/dev");
    if (!dir) {
        LOG_ERROR("无法打开 /dev 目录");
        return ports;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;

        for (const auto& prefix : prefixes) {
            if (name.find(prefix) == 0) {
                ports.push_back("/dev/" + name);
                break;
            }
        }
    }

    closedir(dir);
    return ports;
}

std::unique_ptr<SerialHandle> SerialTool::openPortUnix(const std::string& port, const SerialConfig& config) {
    int fd = ::open(port.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        LOG_ERROR("无法打开串口: " + port);
        return nullptr;
    }

    // 设置为阻塞模式
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);

    auto handle = std::make_unique<UnixSerialHandle>(fd, port);
    if (!handle->configure(config)) {
        return nullptr;
    }

    return handle;
}

#endif // PLATFORM_UNIX

#ifdef PLATFORM_WINDOWS

std::vector<std::string> SerialTool::findPortsWindows() {
    std::vector<std::string> ports;

    // 枚举 COM1-COM32
    for (int i = 1; i <= 32; ++i) {
        std::string port_name = "COM" + std::to_string(i);
        std::string path = "\\\\.\\" + port_name;

        HANDLE handle = CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE,
                                    0, NULL, OPEN_EXISTING, 0, NULL);

        if (handle != INVALID_HANDLE_VALUE) {
            ports.push_back(port_name);
            CloseHandle(handle);
        }
    }

    return ports;
}

std::unique_ptr<SerialHandle> SerialTool::openPortWindows(const std::string& port, const SerialConfig& config) {
    std::string path = "\\\\.\\" + port;

    HANDLE handle = CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE,
                                0, NULL, OPEN_EXISTING, 0, NULL);

    if (handle == INVALID_HANDLE_VALUE) {
        LOG_ERROR("无法打开串口: " + port);
        return nullptr;
    }

    auto serial_handle = std::make_unique<WindowsSerialHandle>(handle, port);
    if (!serial_handle->configure(config)) {
        return nullptr;
    }

    return serial_handle;
}

#endif // PLATFORM_WINDOWS

// ============================================================
// 公共接口实现
// ============================================================

ToolResult SerialTool::listPorts() {
    std::vector<std::string> ports;

#ifdef PLATFORM_UNIX
    ports = findPortsUnix();
#endif
#ifdef PLATFORM_WINDOWS
    ports = findPortsWindows();
#endif

    if (ports.empty()) {
        return ToolResult::ok("未找到可用的串口设备 / No serial ports found");
    }

    std::stringstream ss;
    ss << "可用的串口 / Available serial ports:\n\n";
    for (const auto& port : ports) {
        ss << "  " << port << "\n";
    }

    json meta;
    meta["ports"] = ports;
    meta["count"] = ports.size();

    return ToolResult::ok(ss.str(), meta);
}

ToolResult SerialTool::openPort(const std::string& port, const SerialConfig& config) {
    std::unique_lock<std::shared_mutex> lock(ports_mutex_);

    // 检查是否已经打开
    if (open_ports_.find(port) != open_ports_.end()) {
        return ToolResult::error("串口已打开 / Port already open: " + port);
    }

    std::unique_ptr<SerialHandle> handle;
#ifdef PLATFORM_UNIX
    handle = openPortUnix(port, config);
#endif
#ifdef PLATFORM_WINDOWS
    handle = openPortWindows(port, config);
#endif

    if (!handle) {
        return ToolResult::error("无法打开串口 / Failed to open port: " + port);
    }

    open_ports_[port] = std::move(handle);

    std::stringstream ss;
    ss << "串口已打开 / Port opened: " << port << "\n";
    ss << "波特率 / Baud rate: " << config.baud_rate << "\n";
    ss << "数据位 / Data bits: " << config.data_bits << "\n";
    ss << "停止位 / Stop bits: " << config.stop_bits << "\n";
    ss << "校验位 / Parity: " << config.parity;

    return ToolResult::ok(ss.str());
}

ToolResult SerialTool::closePort(const std::string& port) {
    std::unique_lock<std::shared_mutex> lock(ports_mutex_);

    auto it = open_ports_.find(port);
    if (it == open_ports_.end()) {
        return ToolResult::error("串口未打开 / Port not open: " + port);
    }

    open_ports_.erase(it);
    return ToolResult::ok("串口已关闭 / Port closed: " + port);
}

ToolResult SerialTool::readData(const std::string& port, size_t max_size) {
    std::shared_lock<std::shared_mutex> lock(ports_mutex_);

    auto it = open_ports_.find(port);
    if (it == open_ports_.end()) {
        return ToolResult::error("串口未打开 / Port not open: " + port);
    }

    std::string data;
    if (!it->second->read(data, max_size)) {
        return ToolResult::error("读取失败 / Read failed");
    }

    if (data.empty()) {
        return ToolResult::ok("(无数据 / No data available)");
    }

    // 以十六进制和ASCII形式显示数据
    std::stringstream ss;
    ss << "读取的数据 / Data read (" << data.size() << " bytes):\n\n";

    // 十六进制显示
    ss << "HEX: ";
    for (size_t i = 0; i < data.size() && i < 32; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0')
           << (static_cast<unsigned char>(data[i])) << " ";
    }
    if (data.size() > 32) ss << "...";

    // ASCII显示
    ss << "\nASCII: ";
    for (size_t i = 0; i < data.size() && i < 64; ++i) {
        char c = data[i];
        ss << (std::isprint(c) ? c : '.');
    }
    if (data.size() > 64) ss << "...";

    json meta;
    meta["size"] = data.size();
    meta["hex"] = ss.str();

    // 如果是纯文本，直接返回文本内容
    bool is_text = true;
    for (char c : data) {
        if (c != '\n' && c != '\r' && c != '\t' && !std::isprint(c)) {
            is_text = false;
            break;
        }
    }

    if (is_text) {
        return ToolResult::ok(data, meta);
    }

    return ToolResult::ok(ss.str(), meta);
}

ToolResult SerialTool::writeData(const std::string& port, const std::string& data) {
    std::shared_lock<std::shared_mutex> lock(ports_mutex_);

    auto it = open_ports_.find(port);
    if (it == open_ports_.end()) {
        return ToolResult::error("串口未打开 / Port not open: " + port);
    }

    if (!it->second->write(data)) {
        return ToolResult::error("写入失败 / Write failed");
    }

    std::stringstream ss;
    ss << "已写入 / Wrote " << data.size() << " bytes to " << port;

    json meta;
    meta["size"] = data.size();

    return ToolResult::ok(ss.str(), meta);
}

ToolResult SerialTool::configurePort(const std::string& port, const SerialConfig& config) {
    std::shared_lock<std::shared_mutex> lock(ports_mutex_);

    auto it = open_ports_.find(port);
    if (it == open_ports_.end()) {
        return ToolResult::error("串口未打开 / Port not open: " + port);
    }

    if (!it->second->configure(config)) {
        return ToolResult::error("配置失败 / Configuration failed");
    }

    std::stringstream ss;
    ss << "串口配置已更新 / Port configured: " << port << "\n";
    ss << "波特率 / Baud rate: " << config.baud_rate;

    return ToolResult::ok(ss.str());
}

} // namespace roboclaw
