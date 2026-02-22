#pragma once

#include <nlohmann/json.hpp>
#include <memory>
#include <string>

namespace roboclaw::hal {

/**
 * @brief 电机控制器抽象接口
 *
 * 支持多种电机驱动器：RoboClaw, Sabertooth, L298N, PWM drivers
 */
class IMotorController {
public:
    virtual ~IMotorController() = default;

    /**
     * @brief 初始化电机控制器
     * @param config 配置JSON (端口、地址等)
     * @return 初始化是否成功
     */
    virtual bool initialize(const nlohmann::json& config) = 0;

    /**
     * @brief 设置电机速度
     * @param channel 通道号 (0-based)
     * @param speed 速度值 (0-255 或 0-100)
     */
    virtual void setSpeed(int channel, int speed) = 0;

    /**
     * @brief 设置电机方向
     * @param channel 通道号
     * @param forward true=前进, false=后退
     */
    virtual void setDirection(int channel, bool forward) = 0;

    /**
     * @brief 紧急停止所有电机
     */
    virtual void stop() = 0;

    /**
     * @brief 检查控制器是否已连接
     */
    virtual bool isConnected() const = 0;
};

} // namespace roboclaw::hal
