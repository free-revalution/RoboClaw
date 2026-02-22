#include "motion_skill.h"
#include <thread>
#include <chrono>

namespace roboclaw::skills {

MotionSkill::MotionSkill(std::shared_ptr<hal::IMotorController> motors)
    : motors_(motors) {}

void MotionSkill::forward(int speedPercent, double durationSec) {
    int speed = (speedPercent * 255) / 100;
    motors_->setSpeed(0, speed);
    motors_->setSpeed(1, speed);
    motors_->setDirection(0, true);
    motors_->setDirection(1, true);

    waitForDuration(durationSec);

    if (durationSec > 0) {
        motors_->setSpeed(0, 0);
        motors_->setSpeed(1, 0);
    }
}

void MotionSkill::backward(int speedPercent, double durationSec) {
    int speed = (speedPercent * 255) / 100;
    motors_->setSpeed(0, speed);
    motors_->setSpeed(1, speed);
    motors_->setDirection(0, false);
    motors_->setDirection(1, false);

    waitForDuration(durationSec);

    if (durationSec > 0) {
        motors_->setSpeed(0, 0);
        motors_->setSpeed(1, 0);
    }
}

void MotionSkill::turn(double angleDegrees, int speedPercent) {
    int speed = (speedPercent * 255) / 100;

    if (angleDegrees > 0) {
        motors_->setSpeed(0, speed);
        motors_->setSpeed(1, speed);
        motors_->setDirection(0, true);
        motors_->setDirection(1, false);
    } else {
        motors_->setSpeed(0, speed);
        motors_->setSpeed(1, speed);
        motors_->setDirection(0, false);
        motors_->setDirection(1, true);
    }

    double turnTime = std::abs(angleDegrees) / 90.0 * 0.5;
    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(turnTime * 1000)));

    motors_->setSpeed(0, 0);
    motors_->setSpeed(1, 0);
}

void MotionSkill::stop() {
    motors_->stop();
}

void MotionSkill::waitForDuration(double seconds) {
    if (seconds > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(seconds * 1000)));
    }
}

} // namespace roboclaw::skills
