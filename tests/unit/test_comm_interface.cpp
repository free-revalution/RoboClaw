#include <gtest/gtest.h>
#include "hal/comm.h"
#include <vector>

using namespace roboclaw::hal;

TEST(CommInterface, CanCreateMockComm) {
    class MockComm : public IComm {
    public:
        bool opened = false;
        std::vector<uint8_t> lastWrite;

        bool open(const std::string& port, int baudrate) override {
            opened = true;
            return true;
        }

        bool write(const std::vector<uint8_t>& data) override {
            lastWrite = data;
            return true;
        }

        std::vector<uint8_t> read(int timeout_ms) override {
            return {0x01, 0x02};
        }

        void close() override {
            opened = false;
        }

        bool isOpen() const override {
            return opened;
        }
    };

    MockComm comm;
    EXPECT_TRUE(comm.open("/dev/ttyUSB0", 115200));
    EXPECT_TRUE(comm.opened);

    std::vector<uint8_t> data = {0xAA, 0x55};
    comm.write(data);
    EXPECT_EQ(comm.lastWrite.size(), 2);

    auto readData = comm.read(100);
    EXPECT_EQ(readData.size(), 2);
    EXPECT_EQ(readData[0], 0x01);

    comm.close();
    EXPECT_FALSE(comm.opened);
}
