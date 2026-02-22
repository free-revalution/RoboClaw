#include <gtest/gtest.h>
#include "skills/robot/motion_skill.h"
#include "hal/motor_controller.h"
#include <memory>

using namespace roboclaw::skills;
using namespace roboclaw::hal;

class MockMotorController : public IMotorController {
public:
    int leftSpeed = 0, rightSpeed = 0;
    bool leftForward = true, rightForward = true;

    bool initialize(const nlohmann::json& config) override { return true; }
    void setSpeed(int channel, int speed) override {
        if (channel == 0) leftSpeed = speed;
        else rightSpeed = speed;
    }
    void setDirection(int channel, bool forward) override {
        if (channel == 0) leftForward = forward;
        else rightForward = forward;
    }
    void stop() override {
        leftSpeed = rightSpeed = 0;
    }
    bool isConnected() const override { return true; }
};

TEST(MotionSkill, CanMoveForward) {
    auto motor = std::make_shared<MockMotorController>();
    MotionSkill skill(motor);

    skill.forward(50, 1.0);

    EXPECT_EQ(motor->leftSpeed, 127);  // 50% of 255
    EXPECT_EQ(motor->rightSpeed, 127);
    EXPECT_TRUE(motor->leftForward);
    EXPECT_TRUE(motor->rightForward);
}

TEST(MotionSkill, CanTurn) {
    auto motor = std::make_shared<MockMotorController>();
    MotionSkill skill(motor);

    skill.turn(90, 50);

    EXPECT_NE(motor->leftForward, motor->rightForward);
}

TEST(MotionSkill, CanStop) {
    auto motor = std::make_shared<MockMotorController>();
    MotionSkill skill(motor);

    skill.forward(50);
    skill.stop();

    EXPECT_EQ(motor->leftSpeed, 0);
    EXPECT_EQ(motor->rightSpeed, 0);
}
