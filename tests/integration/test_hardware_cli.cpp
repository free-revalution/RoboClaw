/**
 * @file test_hardware_cli.cpp
 * @brief Integration tests for hardware CLI commands
 */

#include <gtest/gtest.h>
#include "../../src/hal/hardware_config.h"
#include <filesystem>
#include <fstream>

using namespace roboclaw::hal;

class HardwareCLITest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建临时测试目录
        test_dir_ = "/tmp/roboclaw_hardware_test_" + std::to_string(std::random_device{}());
        std::filesystem::create_directories(test_dir_);
        test_config_path_ = test_dir_ + "/hardware.json";
    }

    void TearDown() override {
        // 清理临时目录
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }

    // 创建测试硬件配置文件
    void createTestHardwareConfig() {
        std::ofstream file(test_config_path_);
        file << R"({
            "motors": {
                "left_drive": {
                    "type": "roboclaw",
                    "address": 128,
                    "channel": "M1",
                    "max_current": 20.0,
                    "max_speed": 5000
                },
                "right_drive": {
                    "type": "roboclaw",
                    "address": 128,
                    "channel": "M2",
                    "max_current": 20.0,
                    "max_speed": 5000
                }
            },
            "sensors": {
                "imu": {
                    "type": "mpu6050",
                    "bus": "i2c",
                    "address": "0x68"
                },
                "ultrasonic": {
                    "type": "hc-sr04",
                    "trigger_pin": 23,
                    "echo_pin": 24
                }
            }
        })";
        file.close();
    }

    std::string test_dir_;
    std::string test_config_path_;
};

/**
 * @test Test loading hardware configuration from file
 */
TEST_F(HardwareCLITest, LoadHardwareConfig) {
    createTestHardwareConfig();

    HardwareConfig config;
    bool result = config.loadFromFile(test_config_path_);

    ASSERT_TRUE(result);
    EXPECT_TRUE(config.isLoaded());
}

/**
 * @test Test retrieving motor names
 */
TEST_F(HardwareCLITest, GetMotorNames) {
    createTestHardwareConfig();

    HardwareConfig config;
    config.loadFromFile(test_config_path_);

    auto motors = config.getMotorNames();

    EXPECT_EQ(motors.size(), 2);
    EXPECT_THAT(motors, ::testing::Contains("left_drive"));
    EXPECT_THAT(motors, ::testing::Contains("right_drive"));
}

/**
 * @test Test retrieving sensor names
 */
TEST_F(HardwareCLITest, GetSensorNames) {
    createTestHardwareConfig();

    HardwareConfig config;
    config.loadFromFile(test_config_path_);

    auto sensors = config.getSensorNames();

    EXPECT_EQ(sensors.size(), 2);
    EXPECT_THAT(sensors, ::testing::Contains("imu"));
    EXPECT_THAT(sensors, ::testing::Contains("ultrasonic"));
}

/**
 * @test Test retrieving motor configuration
 */
TEST_F(HardwareCLITest, GetMotorConfig) {
    createTestHardwareConfig();

    HardwareConfig config;
    config.loadFromFile(test_config_path_);

    auto leftMotor = config.getMotorConfig("left_drive");

    EXPECT_FALSE(leftMotor.empty());
    EXPECT_EQ(leftMotor["type"], "roboclaw");
    EXPECT_EQ(leftMotor["address"], 128);
    EXPECT_EQ(leftMotor["channel"], "M1");
    EXPECT_NEAR(leftMotor["max_current"].get<double>(), 20.0, 0.01);
    EXPECT_EQ(leftMotor["max_speed"], 5000);
}

/**
 * @test Test retrieving sensor configuration
 */
TEST_F(HardwareCLITest, GetSensorConfig) {
    createTestHardwareConfig();

    HardwareConfig config;
    config.loadFromFile(test_config_path_);

    auto imu = config.getSensorConfig("imu");

    EXPECT_FALSE(imu.empty());
    EXPECT_EQ(imu["type"], "mpu6050");
    EXPECT_EQ(imu["bus"], "i2c");
    EXPECT_EQ(imu["address"], "0x68");
}

/**
 * @test Test checking motor existence
 */
TEST_F(HardwareCLITest, HasMotor) {
    createTestHardwareConfig();

    HardwareConfig config;
    config.loadFromFile(test_config_path_);

    EXPECT_TRUE(config.hasMotor("left_drive"));
    EXPECT_TRUE(config.hasMotor("right_drive"));
    EXPECT_FALSE(config.hasMotor("nonexistent"));
}

/**
 * @test Test checking sensor existence
 */
TEST_F(HardwareCLITest, HasSensor) {
    createTestHardwareConfig();

    HardwareConfig config;
    config.loadFromFile(test_config_path_);

    EXPECT_TRUE(config.hasSensor("imu"));
    EXPECT_TRUE(config.hasSensor("ultrasonic"));
    EXPECT_FALSE(config.hasSensor("nonexistent"));
}

/**
 * @test Test loading from non-existent file
 */
TEST_F(HardwareCLITest, LoadNonExistentFile) {
    HardwareConfig config;
    bool result = config.loadFromFile("nonexistent.json");

    EXPECT_FALSE(result);
    EXPECT_FALSE(config.isLoaded());
}

/**
 * @test Test getting config from unloaded HardwareConfig
 */
TEST_F(HardwareCLITest, GetConfigFromUnloaded) {
    HardwareConfig config;

    EXPECT_TRUE(config.getMotorNames().empty());
    EXPECT_TRUE(config.getSensorNames().empty());
    EXPECT_FALSE(config.hasMotor("any"));
    EXPECT_FALSE(config.hasSensor("any"));
    EXPECT_TRUE(config.getMotorConfig("any").empty());
    EXPECT_TRUE(config.getSensorConfig("any").empty());
}

/**
 * @test Test getting non-existent motor config
 */
TEST_F(HardwareCLITest, GetNonExistentMotor) {
    createTestHardwareConfig();

    HardwareConfig config;
    config.loadFromFile(test_config_path_);

    auto motor = config.getMotorConfig("nonexistent");

    EXPECT_TRUE(motor.empty());
}

/**
 * @test Test getting config from JSON without motors section
 */
TEST_F(HardwareCLITest, ConfigWithoutMotorsSection) {
    std::ofstream file(test_config_path_);
    file << R"({
        "sensors": {
            "imu": {
                "type": "mpu6050"
            }
        }
    })";
    file.close();

    HardwareConfig config;
    config.loadFromFile(test_config_path_);

    EXPECT_TRUE(config.getMotorNames().empty());
    EXPECT_FALSE(config.hasMotor("any"));
}

/**
 * @test Test getting raw configuration
 */
TEST_F(HardwareCLITest, GetRawConfig) {
    createTestHardwareConfig();

    HardwareConfig config;
    config.loadFromFile(test_config_path_);

    const auto& raw = config.getRawConfig();

    EXPECT_TRUE(raw.contains("motors"));
    EXPECT_TRUE(raw.contains("sensors"));
    EXPECT_EQ(raw["motors"]["left_drive"]["type"], "roboclaw");
}

/**
 * @test Test multiple configuration loads
 */
TEST_F(HardwareCLITest, MultipleConfigurationLoads) {
    createTestHardwareConfig();

    HardwareConfig config;
    ASSERT_TRUE(config.loadFromFile(test_config_path_));

    // 第一次加载
    auto motors1 = config.getMotorNames();
    EXPECT_EQ(motors1.size(), 2);

    // 创建新配置文件
    std::string new_config_path = test_dir_ + "/hardware2.json";
    std::ofstream file(new_config_path);
    file << R"({
        "motors": {
            "extra_motor": {
                "type": "servo",
                "channel": "PWM1"
            }
        }
    })";
    file.close();

    // 加载新配置
    ASSERT_TRUE(config.loadFromFile(new_config_path));

    auto motors2 = config.getMotorNames();
    EXPECT_EQ(motors2.size(), 1);
    EXPECT_THAT(motors2, ::testing::Contains("extra_motor"));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
