/**
 * Copyright (c) 2024 Joe Inman
 *
 * SPDX-License-Identifier: MIT
 */

#include "ov5640/ov5640.hpp"
#include "camera_capture.pio.h"
#include "ov5640_regs.hpp"
#include "ov5640_settings.hpp"
#include <hardware/gpio.h>
#include <hardware/dma.h>
#include <pico/stdlib.h>

namespace jsi {

OV5640::OV5640(SCCB& sccb, const OV5640PinConfig& pins, uint8_t device_addr)
    : sccb_(sccb)
    , device_addr_(device_addr)
    , pins_(pins)
    , pio_(nullptr)
    , sm_(0)
    , offset_(0)
    , dma_channel_(-1)
    , resolution_(OV5640Resolution::QVGA)
    , pixel_format_(OV5640PixelFormat::YUV422)
    , width_(320)
    , height_(240)
{
    // Claim DMA channel
    dma_channel_ = dma_claim_unused_channel(true);
    
    // Initialize PIO
    initPIO();
    
    // Configure GPIO pins
    for (int i = 0; i < 8; i++) {
        gpio_init(pins_.data_pins[i]);
        gpio_set_dir(pins_.data_pins[i], GPIO_IN);
    }
    
    gpio_init(pins_.pclk_pin);
    gpio_set_dir(pins_.pclk_pin, GPIO_IN);
    
    gpio_init(pins_.href_pin);
    gpio_set_dir(pins_.href_pin, GPIO_IN);
    
    gpio_init(pins_.vsync_pin);
    gpio_set_dir(pins_.vsync_pin, GPIO_IN);
    
    // Configure reset pin (active low)
    gpio_init(pins_.reset_pin);
    gpio_set_dir(pins_.reset_pin, GPIO_OUT);
    gpio_put(pins_.reset_pin, 1); // Hold high (inactive)
}

OV5640::~OV5640() {
    // Disable PIO state machine
    if (pio_) {
        pio_sm_set_enabled(pio_, sm_, false);
        pio_remove_program(pio_, &camera_capture_program, offset_);
    }
    
    // Unclaim DMA channel
    if (dma_channel_ >= 0) {
        dma_channel_unclaim(dma_channel_);
    }
}

bool OV5640::detect() {
    uint8_t high = 0, low = 0;
    
    if (!readRegister(CHIP_ID_HIGH, high)) {
        return false;
    }
    
    if (!readRegister(CHIP_ID_LOW, low)) {
        return false;
    }
    
    uint16_t pid = (high << 8) | low;
    return (pid == OV5640_PID);
}

bool OV5640::hardwareReset() {
    // Pulse reset pin low (active low reset)
    gpio_put(pins_.reset_pin, 0);
    sleep_ms(20);  // Hold reset for 20ms
    gpio_put(pins_.reset_pin, 1);
    sleep_ms(20);  // Wait for camera to come out of reset
    return true;
}

bool OV5640::reset() {
    // Software reset
    if (!writeRegister(SYSTEM_CTROL0, 0x82)) {
        return false;
    }
    
    sleep_ms(10);
    
    // Load default register sequence
    if (!writeRegisterSequence(sensor_default_regs)) {
        return false;
    }
    
    return true;
}

bool OV5640::init() {
    // Perform hardware reset
    if (!hardwareReset()) {
        return false;
    }
    
    // Detect sensor
    if (!detect()) {
        return false;
    }
    
    // Reset and load defaults
    if (!reset()) {
        return false;
    }
    
    // Configure format
    if (!setPixelFormat(OV5640PixelFormat::YUV422)) {
        return false;
    }
    
    // Configure resolution
    if (!setResolution(OV5640Resolution::QVGA)) {
        return false;
    }
    
    // Initialize DMA
    initDMA();
    
    // Start streaming
    if (!startStreaming()) {
        return false;
    }
    
    // Give sensor time to stabilize and start outputting
    sleep_ms(200);
    
    return true;
}

bool OV5640::setResolution(OV5640Resolution res) {
    // Get resolution config
    const ResolutionConfig& config = resolution_configs[static_cast<int>(res)];
    
    // Update dimensions
    width_ = config.width;
    height_ = config.height;
    
    // Write timing registers
    if (!writeRegister(X_ADDR_ST_H, (config.start_x >> 8) & 0xFF)) return false;
    if (!writeRegister(X_ADDR_ST_L, config.start_x & 0xFF)) return false;
    if (!writeRegister(Y_ADDR_ST_H, (config.start_y >> 8) & 0xFF)) return false;
    if (!writeRegister(Y_ADDR_ST_L, config.start_y & 0xFF)) return false;
    
    if (!writeRegister(X_ADDR_END_H, (config.end_x >> 8) & 0xFF)) return false;
    if (!writeRegister(X_ADDR_END_L, config.end_x & 0xFF)) return false;
    if (!writeRegister(Y_ADDR_END_H, (config.end_y >> 8) & 0xFF)) return false;
    if (!writeRegister(Y_ADDR_END_L, config.end_y & 0xFF)) return false;
    
    // Write output size
    if (!writeRegister(X_OUTPUT_SIZE_H, (width_ >> 8) & 0xFF)) return false;
    if (!writeRegister(X_OUTPUT_SIZE_L, width_ & 0xFF)) return false;
    if (!writeRegister(Y_OUTPUT_SIZE_H, (height_ >> 8) & 0xFF)) return false;
    if (!writeRegister(Y_OUTPUT_SIZE_L, height_ & 0xFF)) return false;
    
    // Write total size
    if (!writeRegister(X_TOTAL_SIZE_H, (config.total_x >> 8) & 0xFF)) return false;
    if (!writeRegister(X_TOTAL_SIZE_L, config.total_x & 0xFF)) return false;
    if (!writeRegister(Y_TOTAL_SIZE_H, (config.total_y >> 8) & 0xFF)) return false;
    if (!writeRegister(Y_TOTAL_SIZE_L, config.total_y & 0xFF)) return false;
    
    // Write offset
    if (!writeRegister(X_OFFSET_H, (config.offset_x >> 8) & 0xFF)) return false;
    if (!writeRegister(X_OFFSET_L, config.offset_x & 0xFF)) return false;
    if (!writeRegister(Y_OFFSET_H, (config.offset_y >> 8) & 0xFF)) return false;
    if (!writeRegister(Y_OFFSET_L, config.offset_y & 0xFF)) return false;
    
    resolution_ = res;
    return true;
}

bool OV5640::setPixelFormat(OV5640PixelFormat fmt) {
    const uint16_t (*seq)[2] = nullptr;
    
    switch (fmt) {
        case OV5640PixelFormat::YUV422:
            seq = sensor_fmt_yuv422;
            break;
        case OV5640PixelFormat::RGB565:
            seq = sensor_fmt_rgb565;
            break;
        case OV5640PixelFormat::JPEG:
            seq = sensor_fmt_jpeg;
            break;
        case OV5640PixelFormat::RAW:
            seq = sensor_fmt_raw;
            break;
        case OV5640PixelFormat::GRAYSCALE:
            seq = sensor_fmt_grayscale;
            break;
        default:
            return false;
    }
    
    if (!writeRegisterSequence(seq)) {
        return false;
    }
    
    pixel_format_ = fmt;
    return true;
}

bool OV5640::startStreaming() {
    // Configure for DVP (parallel) mode, not MIPI
    
    // Disable MIPI interface
    if (!writeRegister(0x300e, 0x45)) return false;  // MIPI power down
    if (!writeRegister(0x3019, 0x00)) return false;  // MIPI off
    if (!writeRegister(MIPI_CTRL_00, 0x58)) return false;  // MIPI off
    
    // Enable DVP output pads - 0xFF means OUTPUT, 0x00 means INPUT
    // We need these as OUTPUTS so the camera drives them
    if (!writeRegister(PAD_OUTPUT_ENABLE_01, 0xFF)) return false;  // Enable VSYNC, HREF, PCLK outputs
    if (!writeRegister(PAD_OUTPUT_ENABLE_02, 0xFF)) return false;  // Enable data pin outputs
    
    // Set drive capability
    if (!writeRegister(0x302c, 0xc3)) return false;  // Drive capability
    
    // Frame control - ensure frames are not gated
    if (!writeRegister(0x4202, 0x00)) return false;  // Enable frame output (0=enable, 0x0f=disable)
    
    // Configure PLL for DVP mode
    // Set clock to appropriate speed for parallel interface
    if (!writeRegister(SC_PLL_CTRL0, 0x18)) return false;  // PLL charge pump
    if (!writeRegister(SC_PLL_CTRL1, 0x11)) return false;  // System clock divider
    if (!writeRegister(SC_PLL_CTRL2, 0x54)) return false;  // PLL multiplier (84)
    if (!writeRegister(SC_PLL_CTRL3, 0x13)) return false;  // PLL root divider
    if (!writeRegister(0x3108, 0x01)) return false;  // System root divider
    
    // Set PCLK polarity - sample on rising edge
    // Bit 5: PCLK gate enable (0=always output, 1=gate by HREF)
    // Bit 4: PCLK polarity (0=active high, 1=active low)  
    // We want: 0x21 = PCLK not gated, active high
    if (!writeRegister(CLOCK_POL_CONTROL, 0x20)) return false;  // Try without gating first
    
    // Enable test pattern for initial testing
    if (!writeRegister(0x503d, 0x80)) return false;  // Enable test pattern
    if (!writeRegister(0x503e, 0x00)) return false;  // Color bar test pattern
    
    // Wake from standby and start streaming
    if (!writeRegister(SYSTEM_CTROL0, 0x02)) return false;  // Wake from standby first
    sleep_ms(10);
    if (!writeRegister(SYSTEM_CTROL0, 0x00)) return false;  // Start streaming
    
    return true;
}

bool OV5640::stopStreaming() {
    // Write 0x42 to 0x3008 to stop streaming (power down)
    return writeRegister(SYSTEM_CTROL0, 0x42);
}

bool OV5640::isStreaming() {
    uint8_t value = 0;
    if (!readRegister(SYSTEM_CTROL0, value)) {
        return false;
    }
    // Bit 6 = 0 and Bit 7 = 0 means streaming
    // 0x00 or 0x02 are streaming states
    return (value == 0x00 || value == 0x02);
}

bool OV5640::capture(uint8_t* buffer, size_t buffer_size, uint32_t timeout_ms) {
    size_t frame_size = getFrameSize();
    
    if (buffer_size < frame_size) {
        return false;
    }
    
    // Clear PIO FIFO before starting
    pio_sm_clear_fifos(pio_, sm_);
    
    // Get PIO FIFO address
    volatile void* pio_rxf = (volatile void*)&pio_->rxf[sm_];
    
    // Configure DMA transfer
    dma_channel_config c = dma_config_;
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, true);
    
    dma_channel_configure(
        dma_channel_,
        &c,
        buffer,           // Destination
        pio_rxf,         // Source
        frame_size,      // Transfer count
        true             // Start immediately
    );
    
    // Wait for transfer to complete with timeout
    if (timeout_ms == 0) {
        // No timeout - wait indefinitely
        dma_channel_wait_for_finish_blocking(dma_channel_);
        return true;
    } else {
        // Wait with timeout
        absolute_time_t timeout = make_timeout_time_ms(timeout_ms);
        while (dma_channel_is_busy(dma_channel_)) {
            if (time_reached(timeout)) {
                // Timeout - abort DMA transfer
                dma_channel_abort(dma_channel_);
                return false;
            }
            tight_loop_contents();
        }
        return true;
    }
}

size_t OV5640::getFrameSize() const {
    // YUV422 = 2 bytes per pixel
    // RGB565 = 2 bytes per pixel
    // For now, assume 2 bytes per pixel
    return width_ * height_ * 2;
}

bool OV5640::writeRegister(uint16_t reg, uint8_t value) {
    return sccb_.writeRegister(device_addr_, reg, value);
}

bool OV5640::readRegister(uint16_t reg, uint8_t& value) {
    return sccb_.readRegister(device_addr_, reg, value);
}

bool OV5640::writeRegisterSequence(const uint16_t regs[][2]) {
    for (int i = 0; regs[i][0] != REG_LIST_END; i++) {
        uint16_t reg = regs[i][0];
        uint8_t value = regs[i][1];
        
        if (reg == REG_DELAY) {
            sleep_ms(value);
            continue;
        }
        
        if (!writeRegister(reg, value)) {
            return false;
        }
    }
    
    return true;
}

void OV5640::initPIO() {
    // Claim PIO and state machine
    if (!pio_claim_free_sm_and_add_program_for_gpio_range(
            &camera_capture_program, &pio_, &sm_, &offset_,
            pins_.data_pins[0], 8, true)) {
        // Failed to claim - try alternative method
        pio_ = pio0;
        sm_ = pio_claim_unused_sm(pio_, true);
        offset_ = pio_add_program(pio_, &camera_capture_program);
    }
    
    // Initialize the program
    camera_capture_program_init(
        pio_, sm_, offset_,
        pins_.data_pins[0],
        pins_.pclk_pin,
        pins_.href_pin,
        pins_.vsync_pin
    );
}

void OV5640::initDMA() {
    // Get default DMA config
    dma_config_ = dma_channel_get_default_config(dma_channel_);
    
    // Configure transfer size (8-bit)
    channel_config_set_transfer_data_size(&dma_config_, DMA_SIZE_8);
    
    // Configure DREQ (paced by PIO RX FIFO)
    channel_config_set_dreq(&dma_config_, pio_get_dreq(pio_, sm_, false));
}

void OV5640::configureSensor() {
    // Placeholder for future sensor-specific configuration
}

} // namespace jsi
