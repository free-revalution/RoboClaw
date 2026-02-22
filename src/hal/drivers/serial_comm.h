#pragma once

#include "../comm.h"
#include "../hal_exception.h"
#include <string>
#include <vector>
#include <cstdint>

namespace roboclaw::hal::drivers {

/**
 * @brief 串口通信实现
 *
 * 跨平台串口通信：Linux/macOS 使用 termios
 */
class SerialComm : public IComm {
public:
    SerialComm();
    ~SerialComm() override;

    bool open(const std::string& port, int baudrate) override;
    bool write(const std::vector<uint8_t>& data) override;
    std::vector<uint8_t> read(int timeout_ms) override;
    void close() override;
    bool isOpen() const override;

    // 工具方法
    static int validateBaudrate(int baudrate);
    static bool isValidPortName(const std::string& port);

private:
    int fd_;           // 文件描述符
    std::string port_;
    int baudrate_;
    bool open_;

    bool configurePort();
};

} // namespace roboclaw::hal::drivers
