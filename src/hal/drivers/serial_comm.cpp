#include "serial_comm.h"
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdexcept>

namespace roboclaw::hal::drivers {

SerialComm::SerialComm() : fd_(-1), baudrate_(115200), open_(false) {}

SerialComm::~SerialComm() {
    close();
}

bool SerialComm::open(const std::string& port, int baudrate) {
    if (open_) {
        close();
    }

    if (!isValidPortName(port)) {
        throw CommException(port, "Invalid port name");
    }

    fd_ = ::open(port.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd_ < 0) {
        throw CommException(port, "Cannot open port: " + std::string(strerror(errno)));
    }

    baudrate_ = baudrate;
    if (!configurePort()) {
        ::close(fd_);
        fd_ = -1;
        return false;
    }

    port_ = port;
    open_ = true;
    return true;
}

bool SerialComm::configurePort() {
    struct termios options;
    tcgetattr(fd_, &options);

    // 设置波特率
    speed_t speed;
    switch (baudrate_) {
        case 9600:   speed = B9600; break;
        case 19200:  speed = B19200; break;
        case 38400:  speed = B38400; break;
        case 57600:  speed = B57600; break;
        case 115200: speed = B115200; break;
        default:     speed = B9600; break;
    }

    cfsetispeed(&options, speed);
    cfsetospeed(&options, speed);

    // 8N1 配置
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;

    // 启用接收，本地模式
    options.c_cflag |= (CLOCAL | CREAD);

    // 原始输入模式
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    // 原始输出模式
    options.c_oflag &= ~OPOST;

    // 禁用软件流控
    options.c_iflag &= ~(IXON | IXOFF | IXANY);

    // 设置超时
    options.c_cc[VTIME] = 1;
    options.c_cc[VMIN] = 0;

    tcsetattr(fd_, TCSANOW, &options);
    return true;
}

bool SerialComm::write(const std::vector<uint8_t>& data) {
    if (!open_) {
        throw CommException(port_, "Port not open");
    }

    ssize_t written = ::write(fd_, data.data(), data.size());
    if (written < 0) {
        throw CommException(port_, "Write failed: " + std::string(strerror(errno)));
    }

    tcdrain(fd_);
    return written == static_cast<ssize_t>(data.size());
}

std::vector<uint8_t> SerialComm::read(int timeout_ms) {
    if (!open_) {
        throw CommException(port_, "Port not open");
    }

    std::vector<uint8_t> buffer(256);
    ssize_t bytes = ::read(fd_, buffer.data(), buffer.size());
    if (bytes < 0) {
        throw CommException(port_, "Read failed: " + std::string(strerror(errno)));
    }

    buffer.resize(bytes);
    return buffer;
}

void SerialComm::close() {
    if (open_) {
        ::close(fd_);
        fd_ = -1;
        open_ = false;
    }
}

bool SerialComm::isOpen() const {
    return open_;
}

int SerialComm::validateBaudrate(int baudrate) {
    switch (baudrate) {
        case 9600:
        case 19200:
        case 38400:
        case 57600:
        case 115200:
            return baudrate;
        default:
            return -1;
    }
}

bool SerialComm::isValidPortName(const std::string& port) {
    if (port.empty()) return false;
    return port.find("/dev/tty") == 0 || port.find("/dev/ttyUSB") == 0 ||
           port.find("/dev/ttyACM") == 0 || port.find("COM") == 0;
}

} // namespace roboclaw::hal::drivers
