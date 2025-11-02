# OV5640 Camera Library for Raspberry Pi Pico

C++ library for interfacing with the OV5640 5-megapixel camera sensor using PIO and DMA for high-speed parallel data capture.

## Features

- Hardware-accelerated parallel data capture using PIO
- DMA-based frame transfer for minimal CPU overhead
- Support for multiple resolutions (QVGA to UXGA)
- Multiple pixel formats (YUV422, RGB565, JPEG, RAW, Grayscale)
- SCCB (I2C-like) interface for sensor configuration
- Blocking frame capture API

## Hardware Requirements

- Raspberry Pi Pico or compatible RP2040 board
- OV5640 camera module
- 8 contiguous GPIO pins for data (D2-D9)
- 3 additional GPIOs for PCLK, HREF, VSYNC
- 1 GPIO for RESET
- 2 GPIOs for SCCB (I2C)
- External clock source (XCLK) at 24MHz (can be generated via PWM)

## Pin Configuration Example

```
OV5640 D2-D9  → Pico GP11-GP18 (must be contiguous)
OV5640 PCLK   → Pico GP6
OV5640 HREF   → Pico GP3
OV5640 VSYNC  → Pico GP4
OV5640 XCLK   → Pico GP5 (PWM generated)
OV5640 RESET  → Pico GP7
OV5640 SDA    → Pico GP8
OV5640 SCL    → Pico GP9
```

## Usage Example

```cpp
// Initialize I2C and SCCB
i2c_init(i2c0, 100000);
gpio_set_function(8, GPIO_FUNC_I2C);
gpio_set_function(9, GPIO_FUNC_I2C);
jsi::SCCB sccb(writeI2C, readI2C);

// Configure pins
jsi::OV5640PinConfig pins = {
    .data_pins = {11, 12, 13, 14, 15, 16, 17, 18},
    .pclk_pin = 6,
    .href_pin = 3,
    .vsync_pin = 4,
    .xclk_pin = 5,
    .reset_pin = 7
};

// Create camera instance
jsi::OV5640 camera(sccb, pins);

// Initialize camera
if (!camera.detect()) {
    printf("Camera not detected!\n");
    return;
}

if (!camera.init()) {
    printf("Camera initialization failed!\n");
    return;
}

// Capture frame
size_t frame_size = camera.getFrameSize();
std::vector<uint8_t> buffer(frame_size);
if (camera.capture(buffer.data(), buffer.size())) {
    printf("Frame captured: %dx%d\n", camera.getWidth(), camera.getHeight());
}
```

## Supported Resolutions

- QVGA: 320x240
- VGA: 640x480
- SVGA: 800x600
- XGA: 1024x768
- HD: 1280x720
- UXGA: 1600x1200

## Supported Pixel Formats

- YUV422
- RGB565
- JPEG
- RAW
- Grayscale

## Dependencies

- Pico SDK
- SCCB library (included in this project)
- hardware_pio
- hardware_dma
- hardware_gpio

## Architecture

The library uses a three-stage capture pipeline:

1. **PIO State Machine**: Samples 8-bit parallel data on PCLK rising edge, gated by HREF (line valid) and VSYNC (frame valid)
2. **PIO FIFO**: Buffers sampled bytes (8-level deep RX FIFO, joined for 8 words)
3. **DMA Controller**: Transfers data from PIO FIFO to memory buffer at hardware speed

This architecture achieves high-speed capture with minimal CPU intervention.

## License

MIT License - see LICENSE file for details

## References

- OV5640 Datasheet
- [usedbytes/camera-pico-ov7670](https://github.com/usedbytes/camera-pico-ov7670) - PIO camera capture reference
- ESP32 Camera Driver - register configuration reference
