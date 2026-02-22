#include <gtest/gtest.h>
#include "hal/sensor.h"

using namespace roboclaw::hal;

TEST(SensorInterface, CanCreateMockSensor) {
    class MockSensor : public ISensor {
    public:
        nlohmann::json mockData = {{"value", 42}};

        bool initialize(const nlohmann::json& config) override {
            return true;
        }

        nlohmann::json readData() override {
            return mockData;
        }

        bool isAvailable() override {
            return true;
        }

        std::string getSensorType() override {
            return "mock";
        }
    };

    MockSensor sensor;
    nlohmann::json config;
    EXPECT_TRUE(sensor.initialize(config));
    EXPECT_EQ(sensor.readData()["value"], 42);
    EXPECT_TRUE(sensor.isAvailable());
    EXPECT_EQ(sensor.getSensorType(), "mock");
}
