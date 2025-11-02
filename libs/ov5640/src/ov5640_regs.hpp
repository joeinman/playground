/**
 * Copyright (c) 2024 Joe Inman
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef OV5640_REGS_HPP
#define OV5640_REGS_HPP

#include <cstdint>

namespace jsi {

// System control registers
constexpr uint16_t SYSTEM_CTROL0 = 0x3008;

// Chip identification
constexpr uint16_t CHIP_ID_HIGH = 0x300A;
constexpr uint16_t CHIP_ID_LOW = 0x300B;

// Timing registers
constexpr uint16_t X_ADDR_ST_H = 0x3800;
constexpr uint16_t X_ADDR_ST_L = 0x3801;
constexpr uint16_t Y_ADDR_ST_H = 0x3802;
constexpr uint16_t Y_ADDR_ST_L = 0x3803;
constexpr uint16_t X_ADDR_END_H = 0x3804;
constexpr uint16_t X_ADDR_END_L = 0x3805;
constexpr uint16_t Y_ADDR_END_H = 0x3806;
constexpr uint16_t Y_ADDR_END_L = 0x3807;

// Output size registers
constexpr uint16_t X_OUTPUT_SIZE_H = 0x3808;
constexpr uint16_t X_OUTPUT_SIZE_L = 0x3809;
constexpr uint16_t Y_OUTPUT_SIZE_H = 0x380A;
constexpr uint16_t Y_OUTPUT_SIZE_L = 0x380B;

// Total size registers
constexpr uint16_t X_TOTAL_SIZE_H = 0x380C;
constexpr uint16_t X_TOTAL_SIZE_L = 0x380D;
constexpr uint16_t Y_TOTAL_SIZE_H = 0x380E;
constexpr uint16_t Y_TOTAL_SIZE_L = 0x380F;

// Offset registers
constexpr uint16_t X_OFFSET_H = 0x3810;
constexpr uint16_t X_OFFSET_L = 0x3811;
constexpr uint16_t Y_OFFSET_H = 0x3812;
constexpr uint16_t Y_OFFSET_L = 0x3813;

// Increment registers
constexpr uint16_t X_INCREMENT = 0x3814;
constexpr uint16_t Y_INCREMENT = 0x3815;

// Mirror/flip registers
constexpr uint16_t TIMING_TC_REG20 = 0x3820;
constexpr uint16_t TIMING_TC_REG21 = 0x3821;

// Format control registers
constexpr uint16_t FORMAT_CTRL = 0x501F;
constexpr uint16_t FORMAT_CTRL00 = 0x4300;

// ISP control registers
constexpr uint16_t ISP_CONTROL_01 = 0x5001;

// Clock control registers
constexpr uint16_t CLOCK_POL_CONTROL = 0x4740;

// System control registers
constexpr uint16_t PAD_OUTPUT_ENABLE_00 = 0x3016;  // PAD output enable 00
constexpr uint16_t PAD_OUTPUT_ENABLE_01 = 0x3017;  // PAD output enable 01
constexpr uint16_t PAD_OUTPUT_ENABLE_02 = 0x3018;  // PAD output enable 02

// MIPI control
constexpr uint16_t MIPI_CTRL_00 = 0x4800;

// Power control
constexpr uint16_t SC_PLL_CTRL0 = 0x3034;
constexpr uint16_t SC_PLL_CTRL1 = 0x3035;
constexpr uint16_t SC_PLL_CTRL2 = 0x3036;
constexpr uint16_t SC_PLL_CTRL3 = 0x3037;

// Constants
constexpr uint16_t OV5640_PID = 0x5640;
constexpr uint8_t OV5640_ADDR = 0x3C;
constexpr uint16_t REG_DELAY = 0xFFFF;
constexpr uint16_t REG_LIST_END = 0x0000;

} // namespace jsi

#endif // OV5640_REGS_HPP
