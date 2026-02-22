#pragma once

#include "../../hal/motor_controller.h"
#include <memory>

namespace roboclaw::skills {

/**
 * @brief 机器人运动控制技能
 *
 * 支持差速驱动机器人
 */
class MotionSkill {
public:
    explicit MotionSkill(std::shared_ptr<hal::IMotorController> motors);

    /**
     * @brief 前进
     * @param speedPercent 速度百分比 (0-100)
     * @param durationSec 持续时间（秒），0表示持续运动
     */
    void forward(int speedPercent, double durationSec = 0);

    /**
     * @brief 后退
     */
    void backward(int speedPercent, double durationSec = 0);

    /**
     * @brief 转向
     * @param angleDegrees 角度（正数右转，负数左转）
     * @param speedPercent 速度百分比
     */
    void turn(double angleDegrees, int speedPercent);

    /**
     * @brief 紧急停止
     */
    void stop();

private:
    std::shared_ptr<hal::IMotorController> motors_;

    void waitForDuration(double seconds);
};

} // namespace roboclaw::skills
