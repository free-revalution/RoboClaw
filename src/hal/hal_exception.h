#pragma once

#include <stdexcept>
#include <string>

namespace roboclaw::hal {

/**
 * @brief 硬件异常基类
 */
class HardwareException : public std::runtime_error {
public:
    HardwareException(const std::string& component, const std::string& details)
        : std::runtime_error("[" + component + "] " + details) {}
};

/**
 * @brief 通信异常
 */
class CommException : public HardwareException {
public:
    CommException(const std::string& port, const std::string& details)
        : HardwareException("Comm:" + port, details) {}
};

/**
 * @brief 传感器异常
 */
class SensorException : public HardwareException {
public:
    SensorException(const std::string& sensor, const std::string& details)
        : HardwareException("Sensor:" + sensor, details) {}
};

/**
 * @brief 电机控制器异常
 */
class MotorException : public HardwareException {
public:
    MotorException(const std::string& motor, const std::string& details)
        : HardwareException("Motor:" + motor, details) {}
};

} // namespace roboclaw::hal
