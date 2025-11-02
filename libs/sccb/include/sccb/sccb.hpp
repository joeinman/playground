/*
 * Copyright (c) 2025, Joe Inman
 *
 * Licensed under the MIT License.
 * You may obtain a copy of the License at:
 *     https://opensource.org/licenses/MIT
 *
 * This file is part of the SCCB Library.
 */

#pragma once

#include <cstdint>
#include <functional>
#include <vector>

namespace jsi
{

/**
 * @brief SCCB (Serial Camera Control Bus) interface.
 *
 * SCCB is a simplified I2C-like protocol used for camera configuration.
 * Key differences from I2C:
 * - SCCB does NOT use acknowledgment (ACK/NACK) bits
 * - Read operations require a STOP condition between write and read phases (repeated START)
 * - Devices may not respond with ACK, and this is normal behavior
 *
 * The provided I2CWriteFunction and I2CReadFunction callbacks must adhere to SCCB semantics:
 * they should ignore ACK/NACK responses and return false only for actual bus errors
 * (e.g., bus faults, timeouts, hardware failures), not for missing ACKs.
 */
class SCCB
{
public:
    /**
     * @brief Write data to the SCCB bus.
     *
     * SCCB semantics: This function must ignore ACK/NACK responses from the device.
     * Return false only for actual bus errors (timeouts, bus faults, hardware failures),
     * not for missing or invalid ACK/NACK bits.
     *
     * @param device_addr The 7-bit device address
     * @param data Pointer to data buffer to write
     * @param len Number of bytes to write
     * @return true if write succeeded (ignoring ACK/NACK), false on bus error
     */
    using I2CWriteFunction = std::function<bool(uint8_t device_addr, const uint8_t* data, size_t len)>;

    /**
     * @brief Read data from the SCCB bus.
     *
     * SCCB semantics: This function must ignore ACK/NACK responses from the device.
     * Return false only for actual bus errors (timeouts, bus faults, hardware failures),
     * not for missing or invalid ACK/NACK bits.
     *
     * @param device_addr The 7-bit device address
     * @param data Pointer to buffer to store read data
     * @param len Number of bytes to read
     * @return true if read succeeded (ignoring ACK/NACK), false on bus error
     */
    using I2CReadFunction = std::function<bool(uint8_t device_addr, uint8_t* data, size_t len)>;

    /**
     * @brief Construct an SCCB interface using externally managed bus callbacks.
     *
     * The application is responsible for initializing and deinitializing the
     * underlying hardware before interacting with this class.
     */
    SCCB(I2CWriteFunction write_func, I2CReadFunction read_func);
    ~SCCB() = default;

    bool writeRegister(uint8_t device_addr, uint16_t reg_addr, uint8_t value);
    bool writeRegister(uint8_t device_addr, uint16_t reg_addr, const uint8_t* data, size_t len);

    bool readRegister(uint8_t device_addr, uint16_t reg_addr, uint8_t& value);
    bool readRegister(uint8_t device_addr, uint16_t reg_addr, uint8_t* data, size_t len);

private:
    I2CWriteFunction write_func_;
    I2CReadFunction  read_func_;
};

}  // namespace jsi
