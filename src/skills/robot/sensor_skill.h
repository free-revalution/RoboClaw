#pragma once

#include "../../hal/sensor.h"
#include <memory>
#include <string>
#include <unordered_map>

namespace roboclaw::skills {

/**
 * @brief 传感器读取技能
 *
 * 管理多个传感器并提供统一读取接口
 */
class SensorSkill {
public:
    SensorSkill() = default;

    /**
     * @brief 注册传感器
     */
    void registerSensor(const std::string& name, std::shared_ptr<hal::ISensor> sensor);

    /**
     * @brief 读取指定传感器数据
     */
    nlohmann::json readSensor(const std::string& name);

    /**
     * @brief 读取所有传感器数据
     */
    nlohmann::json readAll();

    /**
     * @brief 检查传感器是否可用
     */
    bool isAvailable(const std::string& name);

private:
    std::unordered_map<std::string, std::shared_ptr<hal::ISensor>> sensors_;
};

} // namespace roboclaw::skills
