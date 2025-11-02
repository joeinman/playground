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
 * The provided WriteFunc and ReadFunc callbacks must adhere to SCCB semantics:
 * they should ignore ACK/NACK responses and return false only for actual bus errors
 * (e.g., bus faults, timeouts, hardware failures), not for missing ACKs.
 */
class SCCB
{
public:
    /**
     * @brief Initialize the SCCB bus.
     * @return true if initialization succeeded, false otherwise.
     */
    using InitFunc = std::function<bool()>;

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
     * @param nostop If true, do not send STOP condition after write (for repeated START)
     * @return true if write succeeded (ignoring ACK/NACK), false on bus error
     */
    using WriteFunc = std::function<bool(uint8_t device_addr, const uint8_t* data, size_t len, bool nostop)>;

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
    using ReadFunc = std::function<bool(uint8_t device_addr, uint8_t* data, size_t len)>;

    /**
     * @brief Deinitialize the SCCB bus.
     */
    using DeinitFunc = std::function<void()>;

    SCCB(InitFunc init_func, WriteFunc write_func, ReadFunc read_func, DeinitFunc deinit_func = nullptr);
    ~SCCB();

    bool writeRegister(uint8_t device_addr, uint16_t reg_addr, uint8_t value);
    bool writeRegister(uint8_t device_addr, uint16_t reg_addr, const uint8_t* data, size_t len);

    bool readRegister(uint8_t device_addr, uint16_t reg_addr, uint8_t& value);
    bool readRegister(uint8_t device_addr, uint16_t reg_addr, uint8_t* data, size_t len);

    bool isInitialized() const noexcept;

private:
    InitFunc   init_func_;
    WriteFunc  write_func_;
    ReadFunc   read_func_;
    DeinitFunc deinit_func_;

    bool initialized_;
};

}  // namespace jsi
