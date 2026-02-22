#include <gtest/gtest.h>
#include "hal/hardware_config.h"
#include <fstream>
#include <filesystem>

using namespace roboclaw::hal;

class HardwareConfigTest : public ::testing::Test {
protected:
    std::string testConfigPath_;

    void SetUp() override {
        // Create a temporary config file path
        testConfigPath_ = "/tmp/test_hardware_" + std::to_string(std::random_device{}()) + ".json";
    }

    void TearDown() override {
        // Clean up the temporary config file
        if (std::filesystem::exists(testConfigPath_)) {
            std::filesystem::remove(testConfigPath_);
        }
    }

    void createMotorConfigFile() {
        std::ofstream f(testConfigPath_);
        f << R"({
  "motors": {
    "motor_left": {
      "type": "roboclaw",
      "port": "/dev/ttyUSB0",
      "address": 128,
      "channel": 0,
      "max_speed": 255
    },
    "motor_right": {
      "type": "roboclaw",
      "port": "/dev/ttyUSB0",
      "address": 128,
      "channel": 1,
      "max_speed": 255
    }
  }
})";
        f.close();
    }

    void createSensorConfigFile() {
        std::ofstream f(testConfigPath_);
        f << R"({
  "sensors": {
    "imu": {
      "type": "mpu6050",
      "bus": "i2c",
      "address": 104
    },
    "lidar": {
      "type": "rplidar_a1",
      "port": "/dev/ttyUSB1"
    }
  }
})";
        f.close();
    }

    void createFullConfigFile() {
        std::ofstream f(testConfigPath_);
        f << R"({
  "motors": {
    "motor_left": {
      "type": "roboclaw",
      "port": "/dev/ttyUSB0",
      "address": 128,
      "channel": 0,
      "max_speed": 255
    },
    "motor_right": {
      "type": "roboclaw",
      "port": "/dev/ttyUSB0",
      "address": 128,
      "channel": 1,
      "max_speed": 255
    }
  },
  "sensors": {
    "imu": {
      "type": "mpu6050",
      "bus": "i2c",
      "address": 104
    },
    "lidar": {
      "type": "rplidar_a1",
      "port": "/dev/ttyUSB1"
    }
  }
})";
        f.close();
    }
};

TEST_F(HardwareConfigTest, CanLoadMotorConfig) {
    createMotorConfigFile();

    HardwareConfig config;
    EXPECT_TRUE(config.loadFromFile(testConfigPath_));
    EXPECT_TRUE(config.isLoaded());
}

TEST_F(HardwareConfigTest, CanLoadSensorConfig) {
    createSensorConfigFile();

    HardwareConfig config;
    EXPECT_TRUE(config.loadFromFile(testConfigPath_));
    EXPECT_TRUE(config.isLoaded());
}

TEST_F(HardwareConfigTest, CanLoadFullConfig) {
    createFullConfigFile();

    HardwareConfig config;
    EXPECT_TRUE(config.loadFromFile(testConfigPath_));
    EXPECT_TRUE(config.isLoaded());
}

TEST_F(HardwareConfigTest, LoadNonExistentFileReturnsFalse) {
    HardwareConfig config;
    EXPECT_FALSE(config.loadFromFile("/tmp/nonexistent_file_12345.json"));
    EXPECT_FALSE(config.isLoaded());
}

TEST_F(HardwareConfigTest, CanGetMotorConfig) {
    createFullConfigFile();

    HardwareConfig config;
    ASSERT_TRUE(config.loadFromFile(testConfigPath_));

    auto motorConfig = config.getMotorConfig("motor_left");
    EXPECT_FALSE(motorConfig.empty());
    EXPECT_EQ(motorConfig["type"], "roboclaw");
    EXPECT_EQ(motorConfig["port"], "/dev/ttyUSB0");
    EXPECT_EQ(motorConfig["address"], 128);
    EXPECT_EQ(motorConfig["channel"], 0);
    EXPECT_EQ(motorConfig["max_speed"], 255);
}

TEST_F(HardwareConfigTest, CanGetSensorConfig) {
    createFullConfigFile();

    HardwareConfig config;
    ASSERT_TRUE(config.loadFromFile(testConfigPath_));

    auto sensorConfig = config.getSensorConfig("imu");
    EXPECT_FALSE(sensorConfig.empty());
    EXPECT_EQ(sensorConfig["type"], "mpu6050");
    EXPECT_EQ(sensorConfig["bus"], "i2c");
    EXPECT_EQ(sensorConfig["address"], 104);
}

TEST_F(HardwareConfigTest, GetNonExistentMotorReturnsEmpty) {
    createMotorConfigFile();

    HardwareConfig config;
    ASSERT_TRUE(config.loadFromFile(testConfigPath_));

    auto motorConfig = config.getMotorConfig("nonexistent");
    EXPECT_TRUE(motorConfig.empty());
}

TEST_F(HardwareConfigTest, GetNonExistentSensorReturnsEmpty) {
    createSensorConfigFile();

    HardwareConfig config;
    ASSERT_TRUE(config.loadFromFile(testConfigPath_));

    auto sensorConfig = config.getSensorConfig("nonexistent");
    EXPECT_TRUE(sensorConfig.empty());
}

TEST_F(HardwareConfigTest, CanGetMotorNames) {
    createFullConfigFile();

    HardwareConfig config;
    ASSERT_TRUE(config.loadFromFile(testConfigPath_));

    auto motorNames = config.getMotorNames();
    EXPECT_EQ(motorNames.size(), 2);
    EXPECT_TRUE(std::find(motorNames.begin(), motorNames.end(), "motor_left") != motorNames.end());
    EXPECT_TRUE(std::find(motorNames.begin(), motorNames.end(), "motor_right") != motorNames.end());
}

TEST_F(HardwareConfigTest, CanGetSensorNames) {
    createFullConfigFile();

    HardwareConfig config;
    ASSERT_TRUE(config.loadFromFile(testConfigPath_));

    auto sensorNames = config.getSensorNames();
    EXPECT_EQ(sensorNames.size(), 2);
    EXPECT_TRUE(std::find(sensorNames.begin(), sensorNames.end(), "imu") != sensorNames.end());
    EXPECT_TRUE(std::find(sensorNames.begin(), sensorNames.end(), "lidar") != sensorNames.end());
}

TEST_F(HardwareConfigTest, HasMotorReturnsCorrectResult) {
    createFullConfigFile();

    HardwareConfig config;
    ASSERT_TRUE(config.loadFromFile(testConfigPath_));

    EXPECT_TRUE(config.hasMotor("motor_left"));
    EXPECT_TRUE(config.hasMotor("motor_right"));
    EXPECT_FALSE(config.hasMotor("nonexistent"));
}

TEST_F(HardwareConfigTest, HasSensorReturnsCorrectResult) {
    createFullConfigFile();

    HardwareConfig config;
    ASSERT_TRUE(config.loadFromFile(testConfigPath_));

    EXPECT_TRUE(config.hasSensor("imu"));
    EXPECT_TRUE(config.hasSensor("lidar"));
    EXPECT_FALSE(config.hasSensor("nonexistent"));
}

TEST_F(HardwareConfigTest, GetMotorConfigBeforeLoadReturnsEmpty) {
    HardwareConfig config;
    auto motorConfig = config.getMotorConfig("motor_left");
    EXPECT_TRUE(motorConfig.empty());
}

TEST_F(HardwareConfigTest, GetSensorConfigBeforeLoadReturnsEmpty) {
    HardwareConfig config;
    auto sensorConfig = config.getSensorConfig("imu");
    EXPECT_TRUE(sensorConfig.empty());
}

TEST_F(HardwareConfigTest, GetMotorNamesFromConfigWithoutMotorsReturnsEmpty) {
    createSensorConfigFile();  // Config with only sensors

    HardwareConfig config;
    ASSERT_TRUE(config.loadFromFile(testConfigPath_));

    auto motorNames = config.getMotorNames();
    EXPECT_TRUE(motorNames.empty());
}

TEST_F(HardwareConfigTest, GetSensorNamesFromConfigWithoutSensorsReturnsEmpty) {
    createMotorConfigFile();  // Config with only motors

    HardwareConfig config;
    ASSERT_TRUE(config.loadFromFile(testConfigPath_));

    auto sensorNames = config.getSensorNames();
    EXPECT_TRUE(sensorNames.empty());
}

TEST_F(HardwareConfigTest, CanGetRawConfig) {
    createFullConfigFile();

    HardwareConfig config;
    ASSERT_TRUE(config.loadFromFile(testConfigPath_));

    const auto& rawConfig = config.getRawConfig();
    EXPECT_TRUE(rawConfig.contains("motors"));
    EXPECT_TRUE(rawConfig.contains("sensors"));
}
