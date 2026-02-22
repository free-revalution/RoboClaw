#include <gtest/gtest.h>
#include "hal/drivers/serial_comm.h"

using namespace roboclaw::hal::drivers;

TEST(SerialComm, CanValidateBaudrate) {
    EXPECT_GE(SerialComm::validateBaudrate(115200), 0);
    EXPECT_LT(SerialComm::validateBaudrate(-1), 0);
}

TEST(SerialComm, CanValidatePortName) {
    EXPECT_TRUE(SerialComm::isValidPortName("/dev/ttyUSB0"));
    EXPECT_TRUE(SerialComm::isValidPortName("/dev/ttyACM0"));
    EXPECT_FALSE(SerialComm::isValidPortName(""));
    EXPECT_FALSE(SerialComm::isValidPortName("/invalid/path"));
}
