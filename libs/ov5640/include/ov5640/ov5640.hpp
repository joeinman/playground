/**
 * Copyright (c) 2024 Joe Inman
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef OV5640_HPP
#define OV5640_HPP

#include <cstdint>
#include <hardware/pio.h>
#include <hardware/dma.h>
#include <sccb/sccb.hpp>

namespace jsi {

/**
 * @brief Pin configuration structure for OV5640 camera
 */
struct OV5640PinConfig {
    uint8_t data_pins[8];  // D2-D9 data pins (must be contiguous)
    uint8_t pclk_pin;      // Pixel clock input
    uint8_t href_pin;      // Horizontal reference (line valid)
    uint8_t vsync_pin;     // Vertical sync (frame valid)
    uint8_t xclk_pin;      // External clock output (for reference)
    uint8_t reset_pin;     // Reset pin (active low)
};

/**
 * @brief Supported resolutions for OV5640 camera
 */
enum class OV5640Resolution {
    QVGA,   // 320x240
    VGA,    // 640x480
    SVGA,   // 800x600
    XGA,    // 1024x768
    HD,     // 1280x720
    UXGA    // 1600x1200
};

/**
 * @brief Supported pixel formats for OV5640 camera
 */
enum class OV5640PixelFormat {
    YUV422,
    RGB565,
    JPEG,
    RAW,
    GRAYSCALE
};

/**
 * @brief OV5640 5-megapixel camera driver
 * 
 * Provides interface to OV5640 camera sensor using PIO for parallel
 * data capture and DMA for efficient frame transfer.
 */
class OV5640 {
public:
    /**
     * @brief Construct OV5640 camera driver
     * 
     * @param sccb Reference to SCCB bus interface
     * @param pins Pin configuration structure
     * @param device_addr I2C device address (default 0x3C)
     */
    OV5640(SCCB& sccb, const OV5640PinConfig& pins, uint8_t device_addr = 0x3C);
    
    /**
     * @brief Destructor - cleans up PIO and DMA resources
     */
    ~OV5640();
    
    /**
     * @brief Detect camera by reading PID registers
     * 
     * @return true if camera detected (PID = 0x5640)
     */
    bool detect();
    
    /**
     * @brief Hardware reset via reset pin
     * 
     * @return true on success
     */
    bool hardwareReset();
    
    /**
     * @brief Software reset and load default registers
     * 
     * @return true on success
     */
    bool reset();
    
    /**
     * @brief Complete initialization sequence
     * 
     * Detects camera, resets, configures format and resolution,
     * and sets up DMA transfer.
     * 
     * @return true on success
     */
    bool init();
    
    /**
     * @brief Change camera resolution
     * 
     * @param res Desired resolution
     * @return true on success
     */
    bool setResolution(OV5640Resolution res);
    
    /**
     * @brief Change pixel format
     * 
     * @param fmt Desired pixel format
     * @return true on success
     */
    bool setPixelFormat(OV5640PixelFormat fmt);
    
    /**
     * @brief Start camera streaming
     * 
     * @return true on success
     */
    bool startStreaming();
    
    /**
     * @brief Stop camera streaming
     * 
     * @return true on success
     */
    bool stopStreaming();
    
    /**
     * @brief Check if camera is streaming
     * 
     * @return true if streaming, false otherwise
     */
    bool isStreaming();
    
    /**
     * @brief Capture a single frame (blocking)
     * 
     * @param buffer Destination buffer for frame data
     * @param buffer_size Size of buffer in bytes
     * @param timeout_ms Timeout in milliseconds (0 = no timeout)
     * @return true on success
     */
    bool capture(uint8_t* buffer, size_t buffer_size, uint32_t timeout_ms = 5000);
    
    /**
     * @brief Get current frame width
     * 
     * @return Width in pixels
     */
    uint16_t getWidth() const { return width_; }
    
    /**
     * @brief Get current frame height
     * 
     * @return Height in pixels
     */
    uint16_t getHeight() const { return height_; }
    
    /**
     * @brief Calculate required frame buffer size
     * 
     * @return Size in bytes
     */
    size_t getFrameSize() const;

private:
    // SCCB interface
    SCCB& sccb_;
    uint8_t device_addr_;
    
    // Pin configuration
    OV5640PinConfig pins_;
    
    // PIO resources
    PIO pio_;
    uint sm_;
    uint offset_;
    
    // DMA resources
    int dma_channel_;
    dma_channel_config dma_config_;
    
    // Current configuration
    OV5640Resolution resolution_;
    OV5640PixelFormat pixel_format_;
    uint16_t width_;
    uint16_t height_;
    
    // Helper methods
    bool writeRegister(uint16_t reg, uint8_t value);
    bool readRegister(uint16_t reg, uint8_t& value);
    bool writeRegisterSequence(const uint16_t regs[][2]);
    void initPIO();
    void initDMA();
    void configureSensor();
};

} // namespace jsi

#endif // OV5640_HPP
