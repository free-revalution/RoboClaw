#pragma once

#include <nlohmann/json.hpp>
#include <string>

namespace roboclaw::hal {

/**
 * @brief 传感器抽象接口
 *
 * 支持：IMU (MPU6050), LiDAR, 超声波, 编码器
 */
class ISensor {
public:
    virtual ~ISensor() = default;

    /**
     * @brief 初始化传感器
     */
    virtual bool initialize(const nlohmann::json& config) = 0;

    /**
     * @brief 读取传感器数据
     * @return JSON格式的传感器数据
     */
    virtual nlohmann::json readData() = 0;

    /**
     * @brief 检查传感器是否可用
     */
    virtual bool isAvailable() = 0;

    /**
     * @brief 获取传感器类型
     */
    virtual std::string getSensorType() = 0;
};

} // namespace roboclaw::hal
