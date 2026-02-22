#pragma once

#include <vector>
#include <string>
#include <cstdint>

namespace roboclaw::hal {

/**
 * @brief Communication interface abstraction
 *
 * Supports: Serial, I2C, SPI, CAN
 */
class IComm {
public:
    virtual ~IComm() = default;

    /**
     * @brief Open communication port
     * @param port Port name
     * @param baudrate Baud rate
     * @return Success status
     */
    virtual bool open(const std::string& port, int baudrate) = 0;

    /**
     * @brief Write data to port
     * @param data Data to write
     * @return Success status
     */
    virtual bool write(const std::vector<uint8_t>& data) = 0;

    /**
     * @brief Read data from port
     * @param timeout_ms Timeout in milliseconds
     * @return Read data
     */
    virtual std::vector<uint8_t> read(int timeout_ms) = 0;

    /**
     * @brief Close communication port
     */
    virtual void close() = 0;

    /**
     * @brief Check if port is open
     * @return Open status
     */
    virtual bool isOpen() const = 0;
};

} // namespace roboclaw::hal
