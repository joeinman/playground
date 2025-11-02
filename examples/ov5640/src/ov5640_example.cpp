/**
 * Copyright (c) 2024 Joe Inman
 *
 * SPDX-License-Identifier: MIT
 */

#include <cstdio>
#include <vector>
#include <pico/stdlib.h>
#include <hardware/i2c.h>
#include <hardware/pwm.h>
#include <hardware/clocks.h>
#include <sccb/sccb.hpp>
#include <ov5640/ov5640.hpp>

// I2C Configuration
constexpr uint32_t I2C_FREQ       = 100000;  // 100kHz
constexpr uint8_t  I2C_SDA_PIN    = 8;
constexpr uint8_t  I2C_SCL_PIN    = 9;
constexpr uint32_t I2C_TIMEOUT_US = 1000;

// Camera Configuration
constexpr uint8_t  CAMERA_ADDR = 0x3C;
constexpr uint8_t  XCLK_PIN    = 5;
constexpr uint32_t XCLK_FREQ   = 24000000;  // 24MHz

/**
 * @brief Initialize camera clock (XCLK) using PWM
 *
 * Generates a 24MHz clock signal on GP5 for the camera
 */
void initCameraClock()
{
    // Set GPIO function to PWM
    gpio_set_function(XCLK_PIN, GPIO_FUNC_PWM);

    // Get PWM slice for this pin
    uint slice_num = pwm_gpio_to_slice_num(XCLK_PIN);

    // Calculate divider for desired frequency
    // System clock is typically 125MHz
    uint32_t sys_clock = clock_get_hz(clk_sys);

    // PWM frequency = sys_clock / (wrap * divider)
    // We want 24MHz, so: divider = sys_clock / (wrap * 24MHz)
    // Using wrap = 2 for 50% duty cycle square wave
    uint16_t wrap    = 2;
    float    divider = (float) sys_clock / (float) (wrap * XCLK_FREQ);

    // Configure PWM
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, divider);
    pwm_config_set_wrap(&config, wrap - 1);
    pwm_init(slice_num, &config, false);

    // Set duty cycle to 50%
    pwm_set_gpio_level(XCLK_PIN, wrap / 2);

    // Enable PWM
    pwm_set_enabled(slice_num, true);

    printf("Camera clock initialized: %lu Hz\n", sys_clock / (uint32_t) (wrap * divider));
}

/**
 * @brief Initialize I2C bus for SCCB communication
 */
void initI2C()
{
    i2c_init(i2c0, I2C_FREQ);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    printf("I2C initialized at %u Hz\n", I2C_FREQ);
}

/**
 * @brief I2C write callback for SCCB
 */
bool writeI2C(uint8_t device_addr, const uint8_t* data, size_t len)
{
    if (data == nullptr || len == 0)
    {
        return false;
    }

    int result = i2c_write_timeout_us(i2c0, device_addr, data, len, false, I2C_TIMEOUT_US);
    return result >= 0 && static_cast<size_t>(result) == len;
}

/**
 * @brief I2C read callback for SCCB
 */
bool readI2C(uint8_t device_addr, uint8_t* data, size_t len)
{
    if (data == nullptr || len == 0)
    {
        return false;
    }

    int result = i2c_read_timeout_us(i2c0, device_addr, data, len, false, I2C_TIMEOUT_US);
    return result >= 0 && static_cast<size_t>(result) == len;
}

/**
 * @brief Check if camera signals are active
 */
void checkCameraSignals(const jsi::OV5640PinConfig& pins)
{
    printf("\nChecking camera signals...\n");

    // Check XCLK is toggling
    printf("  XCLK  (GP%d): ", pins.xclk_pin);
    int  xclk_changes = 0;
    bool last_xclk    = gpio_get(pins.xclk_pin);
    for (int i = 0; i < 100; i++)
    {
        bool xclk = gpio_get(pins.xclk_pin);
        if (xclk != last_xclk)
        {
            xclk_changes++;
            last_xclk = xclk;
        }
        sleep_us(1);
    }
    printf("%d transitions in 100us\n", xclk_changes);

    // Check VSYNC
    bool vsync = gpio_get(pins.vsync_pin);
    printf("  VSYNC (GP%d): %s\n", pins.vsync_pin, vsync ? "HIGH" : "LOW");

    // Check HREF toggling
    printf("  HREF  (GP%d): ", pins.href_pin);
    int  href_changes = 0;
    bool last_href    = gpio_get(pins.href_pin);
    for (int i = 0; i < 1000; i++)  // Check over longer period for HREF
    {
        bool href = gpio_get(pins.href_pin);
        if (href != last_href)
        {
            href_changes++;
            last_href = href;
        }
        sleep_us(1);
    }
    printf("%d transitions in 1ms\n", href_changes);

    // Sample PCLK a few times to see if it's toggling
    printf("  PCLK  (GP%d): ", pins.pclk_pin);
    int  pclk_changes = 0;
    bool last_pclk    = gpio_get(pins.pclk_pin);
    for (int i = 0; i < 100; i++)
    {
        bool pclk = gpio_get(pins.pclk_pin);
        if (pclk != last_pclk)
        {
            pclk_changes++;
            last_pclk = pclk;
        }
        sleep_us(1);
    }
    printf("%d transitions in 100us\n", pclk_changes);

    // Check data pins
    printf("  Data pins (GP%d-GP%d): 0x%02X\n",
           pins.data_pins[0],
           pins.data_pins[7],
           (gpio_get(pins.data_pins[7]) << 7) | (gpio_get(pins.data_pins[6]) << 6) |
               (gpio_get(pins.data_pins[5]) << 5) | (gpio_get(pins.data_pins[4]) << 4) |
               (gpio_get(pins.data_pins[3]) << 3) | (gpio_get(pins.data_pins[2]) << 2) |
               (gpio_get(pins.data_pins[1]) << 1) | gpio_get(pins.data_pins[0]));
    printf("\n");
}

/**
 * @brief Read and display key camera registers
 */
void checkCameraRegisters(jsi::SCCB& sccb, uint8_t addr)
{
    printf("\nReading key camera registers...\n");

    uint8_t data[2];

    // System control
    if (sccb.readRegister(addr, 0x3008, data, 1))
    {
        printf("  System Control (0x3008): 0x%02X ", data[0]);
        if (data[0] == 0x00)
            printf("(Streaming)\n");
        else if (data[0] == 0x02)
            printf("(Wake from standby)\n");
        else if (data[0] == 0x42)
            printf("(Power down)\n");
        else if (data[0] == 0x82)
            printf("(Software reset)\n");
        else
            printf("(Unknown)\n");
    }

    // MIPI control
    if (sccb.readRegister(addr, 0x300e, data, 1))
    {
        printf("  MIPI Control (0x300e): 0x%02X\n", data[0]);
    }

    // MIPI control 00
    if (sccb.readRegister(addr, 0x4800, data, 1))
    {
        printf("  MIPI Control 00 (0x4800): 0x%02X\n", data[0]);
    }

    // PAD output enable
    if (sccb.readRegister(addr, 0x3017, data, 1))
    {
        printf("  PAD Output Enable 01 (0x3017): 0x%02X\n", data[0]);
    }
    if (sccb.readRegister(addr, 0x3018, data, 1))
    {
        printf("  PAD Output Enable 02 (0x3018): 0x%02X\n", data[0]);
    }

    // Clock polarity
    if (sccb.readRegister(addr, 0x4740, data, 1))
    {
        printf("  Clock Polarity (0x4740): 0x%02X\n", data[0]);
    }

    // Test pattern
    if (sccb.readRegister(addr, 0x503d, data, 1))
    {
        printf("  Test Pattern Enable (0x503d): 0x%02X %s\n", data[0], (data[0] & 0x80) ? "(Enabled)" : "(Disabled)");
    }

    // PLL settings
    if (sccb.readRegister(addr, 0x3034, data, 1))
    {
        printf("  PLL CTRL0 (0x3034): 0x%02X\n", data[0]);
    }
    if (sccb.readRegister(addr, 0x3035, data, 1))
    {
        printf("  PLL CTRL1 (0x3035): 0x%02X\n", data[0]);
    }

    printf("\n");
}

int main()
{
    // Initialize stdio
    stdio_init_all();

    // Wait for USB connection
    sleep_ms(2000);

    printf("\n=== OV5640 Camera Example ===\n");
    printf("Initializing camera system...\n\n");

    // Initialize camera clock
    printf("Starting camera clock (XCLK)...\n");
    initCameraClock();
    sleep_ms(100);  // Let clock stabilize

    // Initialize I2C bus
    printf("Initializing I2C bus...\n");
    initI2C();

    // Create SCCB instance
    jsi::SCCB sccb(writeI2C, readI2C);
    printf("SCCB interface created\n\n");

    // Configure camera pins
    jsi::OV5640PinConfig pins = {
        .data_pins = {11, 12, 13, 14, 15, 16, 17, 18},  // D2-D9: GP11-GP18
        .pclk_pin  = 6,                                 // PCLK: GP6
        .href_pin  = 3,                                 // HREF: GP3
        .vsync_pin = 4,                                 // VSYNC: GP4
        .xclk_pin  = 5,                                 // XCLK: GP5 (reference)
        .reset_pin = 7                                  // RESET: GP7
    };

    // Create camera instance
    printf("Creating OV5640 camera instance...\n");
    jsi::OV5640 camera(sccb, pins, CAMERA_ADDR);
    printf("Camera instance created\n\n");

    // Detect camera
    printf("Detecting camera...\n");
    if (!camera.detect())
    {
        printf("ERROR: Camera not detected!\n");
        printf("Check connections and I2C address (0x%02X)\n", CAMERA_ADDR);
        while (true)
        {
            tight_loop_contents();
        }
    }
    printf("Camera detected successfully (PID: 0x5640)\n\n");

    // Initialize camera
    printf("Initializing camera...\n");
    if (!camera.init())
    {
        printf("ERROR: Camera initialization failed!\n");
        while (true)
        {
            tight_loop_contents();
        }
    }
    printf("Camera initialized successfully\n");
    printf("Resolution: %dx%d\n", camera.getWidth(), camera.getHeight());
    printf("Frame size: %zu bytes\n", camera.getFrameSize());
    printf("Streaming: %s\n\n", camera.isStreaming() ? "YES" : "NO");

    // Check register values
    checkCameraRegisters(sccb, CAMERA_ADDR);

    // Check camera signals
    checkCameraSignals(pins);

    // Allocate frame buffer
    size_t               frame_size = camera.getFrameSize();
    std::vector<uint8_t> buffer(frame_size);
    printf("Frame buffer allocated: %zu bytes\n\n", frame_size);

    // Capture frame with 5 second timeout
    printf("Capturing frame (timeout: 5 seconds)...\n");
    if (!camera.capture(buffer.data(), buffer.size()))
    {
        printf("ERROR: Frame capture failed or timed out!\n");
        printf("This could mean:\n");
        printf("  - Camera is not outputting video signals\n");
        printf("  - VSYNC/HREF/PCLK connections are incorrect\n");
        printf("  - Camera clock (XCLK) is not running\n");
        while (true)
        {
            tight_loop_contents();
        }
    }
    printf("Frame captured successfully!\n\n");

    // Print frame info
    printf("Frame Information:\n");
    printf("  Width:  %u pixels\n", camera.getWidth());
    printf("  Height: %u pixels\n", camera.getHeight());
    printf("  Size:   %zu bytes\n", buffer.size());
    printf("\n");

    // Print first 64 bytes as hex dump
    printf("First 64 bytes of frame data:\n");
    for (size_t i = 0; i < 64 && i < buffer.size(); i++)
    {
        if (i % 16 == 0)
        {
            printf("%04zx: ", i);
        }
        printf("%02x ", buffer[i]);
        if ((i + 1) % 16 == 0)
        {
            printf("\n");
        }
    }
    printf("\n\n");

    printf("Example complete! Camera is operational.\n");
    printf("You can now modify this code to process frames.\n");

    // Keep running
    while (true)
    {
        tight_loop_contents();
    }

    return 0;
}
