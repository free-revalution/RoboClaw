#include <gtest/gtest.h>
#include "skills/robot/sensor_skill.h"
#include "hal/sensor.h"
#include <memory>

using namespace roboclaw::skills;
using namespace roboclaw::hal;

class MockIMU : public ISensor {
public:
    nlohmann::json data = {{"accel", {{"x", 0}, {"y", 0}, {"z", 9.8}}}};

    bool initialize(const nlohmann::json& config) override { return true; }
    nlohmann::json readData() override { return data; }
    bool isAvailable() override { return true; }
    std::string getSensorType() override { return "imu"; }
};

TEST(SensorSkill, CanReadSensor) {
    auto imu = std::make_shared<MockIMU>();
    SensorSkill skill;

    skill.registerSensor("imu", imu);
    auto data = skill.readSensor("imu");

    EXPECT_TRUE(data.contains("accel"));
    EXPECT_EQ(data["accel"]["z"], 9.8);
}

TEST(SensorSkill, CanReadAll) {
    auto imu1 = std::make_shared<MockIMU>();
    auto imu2 = std::make_shared<MockIMU>();
    SensorSkill skill;

    skill.registerSensor("imu1", imu1);
    skill.registerSensor("imu2", imu2);

    auto allData = skill.readAll();
    EXPECT_TRUE(allData.contains("imu1"));
    EXPECT_TRUE(allData.contains("imu2"));
}
