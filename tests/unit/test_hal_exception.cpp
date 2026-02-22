#include <gtest/gtest.h>
#include "hal/hal_exception.h"

using namespace roboclaw::hal;

TEST(HALException, CanThrowAndCatch) {
    EXPECT_THROW(
        throw HardwareException("Motor", "Connection failed"),
        HardwareException
    );
}

TEST(HALException, CommExceptionFormatsMessage) {
    try {
        throw CommException("/dev/ttyUSB0", "Port not found");
    } catch (const HardwareException& e) {
        std::string msg(e.what());
        EXPECT_NE(msg.find("[Comm:/dev/ttyUSB0]"), std::string::npos);
        EXPECT_NE(msg.find("Port not found"), std::string::npos);
    }
}

TEST(HALException, SensorExceptionFormatsMessage) {
    try {
        throw SensorException("IMU", "Read failed");
    } catch (const HardwareException& e) {
        std::string msg(e.what());
        EXPECT_NE(msg.find("[Sensor:IMU]"), std::string::npos);
        EXPECT_NE(msg.find("Read failed"), std::string::npos);
    }
}

TEST(HALException, MotorExceptionFormatsMessage) {
    try {
        throw MotorException("LeftMotor", "Overload");
    } catch (const HardwareException& e) {
        std::string msg(e.what());
        EXPECT_NE(msg.find("[Motor:LeftMotor]"), std::string::npos);
        EXPECT_NE(msg.find("Overload"), std::string::npos);
    }
}
