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
#include <utility>

namespace jsi
{

SCCB::SCCB(I2CWriteFunction write_func, I2CReadFunction read_func) :
    write_func_(std::move(write_func)), read_func_(std::move(read_func))
{}

bool SCCB::writeRegister(uint8_t device_addr, uint16_t reg_addr, uint8_t value)
{
    if (!write_func_)
    {
        return false;
    }

    const uint8_t buffer[3] = {
        static_cast<uint8_t>((reg_addr >> 8U) & 0xFFU),
        static_cast<uint8_t>(reg_addr & 0xFFU),
        value,
    };

    return write_func_(device_addr, buffer, 3);
}

bool SCCB::writeRegister(uint8_t device_addr, uint16_t reg_addr, const uint8_t* data, size_t len)
{
    if (!write_func_ || data == nullptr || len == 0)
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

    return write_func_(device_addr, buffer.data(), buffer.size());
}

bool SCCB::readRegister(uint8_t device_addr, uint16_t reg_addr, uint8_t& value)
{
    if (!write_func_ || !read_func_)
    {
        return false;
    }

    const uint8_t buffer[2] = {
        static_cast<uint8_t>((reg_addr >> 8U) & 0xFFU),
        static_cast<uint8_t>(reg_addr & 0xFFU),
    };

    if (!write_func_(device_addr, buffer, 2))
    {
        return false;
    }

    return read_func_(device_addr, &value, 1);
}

bool SCCB::readRegister(uint8_t device_addr, uint16_t reg_addr, uint8_t* data, size_t len)
{
    if (!write_func_ || !read_func_ || data == nullptr || len == 0)
    {
        return false;
    }

    const uint8_t buffer[2] = {
        static_cast<uint8_t>((reg_addr >> 8U) & 0xFFU),
        static_cast<uint8_t>(reg_addr & 0xFFU),
    };

    if (!write_func_(device_addr, buffer, 2))
    {
        return false;
    }

    return read_func_(device_addr, data, len);
}
}  // namespace jsi
