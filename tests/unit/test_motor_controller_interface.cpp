#include <gtest/gtest.h>
#include "hal/motor_controller.h"
#include <nlohmann/json.hpp>

using namespace roboclaw::hal;
using json = nlohmann::json;

TEST(MotorControllerInterface, CanCreateMockImplementation) {
    class MockMotorController : public IMotorController {
    public:
        bool initialized = false;
        int lastSpeed = 0;
        bool lastDirection = true;

        bool initialize(const json& config) override {
            initialized = true;
            return true;
        }

        void setSpeed(int channel, int speed) override {
            lastSpeed = speed;
        }

        void setDirection(int channel, bool forward) override {
            lastDirection = forward;
        }

        void stop() override {
            lastSpeed = 0;
        }

        bool isConnected() const override {
            return initialized;
        }
    };

    MockMotorController mock;
    json config = {{"port", "/dev/ttyUSB0"}};

    EXPECT_TRUE(mock.initialize(config));
    EXPECT_TRUE(mock.initialized);
    EXPECT_EQ(mock.lastSpeed, 0);
}

TEST(MotorControllerInterface, SetSpeedUpdatesSpeed) {
    class MockMotorController : public IMotorController {
    public:
        bool initialized = false;
        int lastSpeed = 0;
        bool lastDirection = true;
        int lastChannel = -1;

        bool initialize(const json& config) override {
            initialized = true;
            return true;
        }

        void setSpeed(int channel, int speed) override {
            lastChannel = channel;
            lastSpeed = speed;
        }

        void setDirection(int channel, bool forward) override {
            lastDirection = forward;
        }

        void stop() override {
            lastSpeed = 0;
        }

        bool isConnected() const override {
            return initialized;
        }
    };

    MockMotorController mock;
    json config = {{"port", "/dev/ttyUSB0"}};
    mock.initialize(config);

    mock.setSpeed(0, 128);
    EXPECT_EQ(mock.lastSpeed, 128);
    EXPECT_EQ(mock.lastChannel, 0);

    mock.setSpeed(1, 255);
    EXPECT_EQ(mock.lastSpeed, 255);
    EXPECT_EQ(mock.lastChannel, 1);
}

TEST(MotorControllerInterface, SetDirectionUpdatesDirection) {
    class MockMotorController : public IMotorController {
    public:
        bool initialized = false;
        bool lastDirection = true;
        int lastChannel = -1;

        bool initialize(const json& config) override {
            initialized = true;
            return true;
        }

        void setSpeed(int channel, int speed) override {
            (void)speed;
        }

        void setDirection(int channel, bool forward) override {
            lastChannel = channel;
            lastDirection = forward;
        }

        void stop() override {
        }

        bool isConnected() const override {
            return initialized;
        }
    };

    MockMotorController mock;
    json config = {{"port", "/dev/ttyUSB0"}};
    mock.initialize(config);

    mock.setDirection(0, false);
    EXPECT_FALSE(mock.lastDirection);
    EXPECT_EQ(mock.lastChannel, 0);

    mock.setDirection(1, true);
    EXPECT_TRUE(mock.lastDirection);
    EXPECT_EQ(mock.lastChannel, 1);
}

TEST(MotorControllerInterface, StopResetsSpeed) {
    class MockMotorController : public IMotorController {
    public:
        bool initialized = false;
        int lastSpeed = 100;

        bool initialize(const json& config) override {
            initialized = true;
            return true;
        }

        void setSpeed(int channel, int speed) override {
            lastSpeed = speed;
            (void)channel;
        }

        void setDirection(int channel, bool forward) override {
            (void)forward;
            (void)channel;
        }

        void stop() override {
            lastSpeed = 0;
        }

        bool isConnected() const override {
            return initialized;
        }
    };

    MockMotorController mock;
    json config = {{"port", "/dev/ttyUSB0"}};
    mock.initialize(config);

    mock.setSpeed(0, 200);
    EXPECT_EQ(mock.lastSpeed, 200);

    mock.stop();
    EXPECT_EQ(mock.lastSpeed, 0);
}

TEST(MotorControllerInterface, IsConnectedReturnsInitializationState) {
    class MockMotorController : public IMotorController {
    public:
        bool initialized = false;

        bool initialize(const json& config) override {
            initialized = true;
            (void)config;
            return true;
        }

        void setSpeed(int channel, int speed) override {
            (void)speed;
            (void)channel;
        }

        void setDirection(int channel, bool forward) override {
            (void)forward;
            (void)channel;
        }

        void stop() override {
        }

        bool isConnected() const override {
            return initialized;
        }
    };

    MockMotorController mock;

    EXPECT_FALSE(mock.isConnected());

    json config = {{"port", "/dev/ttyUSB0"}};
    mock.initialize(config);

    EXPECT_TRUE(mock.isConnected());
}
