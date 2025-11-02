/*
 * Copyright (c) 2025, Joe Inman
 *
 * Licensed under the MIT License.
 * You may obtain a copy of the License at:
 *     https://opensource.org/licenses/MIT
 *
 * This file is part of the SCCB Library.
 */

#include "sccb/sccb.hpp"

namespace jsi
{

SCCB::SCCB(InitFunc init_func, WriteFunc write_func, ReadFunc read_func, DeinitFunc deinit_func) :
    init_func_(init_func),
    write_func_(write_func),
    read_func_(read_func),
    deinit_func_(deinit_func),
    initialized_(false)
{
    if (init_func_)
    {
        initialized_ = init_func_();
    }
    else
    {
        // If no init function provided, assume bus is already initialized externally
        initialized_ = true;
    }
}

SCCB::~SCCB()
{
    // Only deinitialize if we successfully initialized (and a deinit function was provided)
    // Skip deinit if SCCB did not perform initialization (e.g., externally initialized bus)
    if (deinit_func_ && initialized_ && init_func_)
    {
        deinit_func_();
    }
}

bool SCCB::writeRegister(uint8_t device_addr, uint16_t reg_addr, uint8_t value)
{
    if (!initialized_ || !write_func_)
    {
        return false;
    }

    const uint8_t buffer[3] = {
        static_cast<uint8_t>((reg_addr >> 8U) & 0xFFU),
        static_cast<uint8_t>(reg_addr & 0xFFU),
        value,
    };

    return write_func_(device_addr, buffer, 3, false);
}

bool SCCB::writeRegister(uint8_t device_addr, uint16_t reg_addr, const uint8_t* data, size_t len)
{
    if (!initialized_ || !write_func_ || data == nullptr || len == 0)
    {
        return false;
    }

    std::vector<uint8_t> buffer(2U + len);
    buffer[0] = static_cast<uint8_t>((reg_addr >> 8U) & 0xFFU);
    buffer[1] = static_cast<uint8_t>(reg_addr & 0xFFU);

    for (size_t index = 0; index < len; ++index)
    {
        buffer[index + 2U] = data[index];
    }

    return write_func_(device_addr, buffer.data(), buffer.size(), false);
}

bool SCCB::readRegister(uint8_t device_addr, uint16_t reg_addr, uint8_t& value)
{
    if (!initialized_ || !write_func_ || !read_func_)
    {
        return false;
    }

    const uint8_t buffer[2] = {
        static_cast<uint8_t>((reg_addr >> 8U) & 0xFFU),
        static_cast<uint8_t>(reg_addr & 0xFFU),
    };

    if (!write_func_(device_addr, buffer, 2, false))
    {
        return false;
    }

    return read_func_(device_addr, &value, 1);
}

bool SCCB::readRegister(uint8_t device_addr, uint16_t reg_addr, uint8_t* data, size_t len)
{
    if (!initialized_ || !write_func_ || !read_func_ || data == nullptr || len == 0)
    {
        return false;
    }

    const uint8_t buffer[2] = {
        static_cast<uint8_t>((reg_addr >> 8U) & 0xFFU),
        static_cast<uint8_t>(reg_addr & 0xFFU),
    };

    if (!write_func_(device_addr, buffer, 2, false))
    {
        return false;
    }

    return read_func_(device_addr, data, len);
}

bool SCCB::isInitialized() const noexcept
{
    return initialized_;
}

}  // namespace jsi
