#include <hardware/clocks.h>
#include <hardware/gpio.h>
#include <hardware/i2c.h>
#include <hardware/pwm.h>
#include <pico/stdlib.h>

#include <array>
#include <cstdio>

#include <sccb/sccb.hpp>

constexpr uint     I2CFrequencyHz = 100 * 1000;
constexpr uint8_t  I2CSDAPin      = 8;
constexpr uint8_t  I2CSCLPin      = 9;
constexpr uint32_t I2CTimeoutUS   = 1'000;

constexpr uint     XCLKPin         = 5;
constexpr uint32_t XCLKFrequencyHz = 24'000'000;
constexpr uint16_t XCLKWrap        = 1;

constexpr uint16_t OV5640ChipIDRegister  = 0x300A;
constexpr uint16_t OV5640SYSCTRLRegister = 0x3008;

constexpr std::array<uint8_t, 2> Ov5640CandidateAddresses = {0x3C, 0x3D};

void initCameraClock()
{
    const uint slice = pwm_gpio_to_slice_num(XCLKPin);

    gpio_set_function(XCLKPin, GPIO_FUNC_PWM);

    pwm_config  config  = pwm_get_default_config();
    const float clk_div = static_cast<float>(clock_get_hz(clk_sys)) /
                          (static_cast<float>(XCLKFrequencyHz) * static_cast<float>(XCLKWrap + 1));

    pwm_config_set_clkdiv(&config, clk_div);
    pwm_config_set_wrap(&config, XCLKWrap);
    pwm_init(slice, &config, false);

    const uint16_t duty_level = (XCLKWrap + 1) / 2;
    pwm_set_gpio_level(XCLKPin, duty_level);
    pwm_set_enabled(slice, true);
    sleep_ms(5);
}

bool initI2C()
{
    i2c_init(i2c0, I2CFrequencyHz);
    gpio_set_function(I2CSDAPin, GPIO_FUNC_I2C);
    gpio_set_function(I2CSCLPin, GPIO_FUNC_I2C);
    gpio_pull_up(I2CSDAPin);
    gpio_pull_up(I2CSCLPin);
    sleep_ms(10);
    return true;
}

bool writeI2C(uint8_t device_addr, const uint8_t* data, size_t len)
{
    if (data == nullptr || len == 0)
    {
        return false;
    }

    const int result = i2c_write_timeout_us(i2c0, device_addr, data, len, false, I2CTimeoutUS);
    return result >= 0 && static_cast<size_t>(result) == len;
}

bool readI2C(uint8_t device_addr, uint8_t* data, size_t len)
{
    if (data == nullptr || len == 0)
    {
        return false;
    }

    const int result = i2c_read_timeout_us(i2c0, device_addr, data, len, false, I2CTimeoutUS);
    return result >= 0 && static_cast<size_t>(result) == len;
}

void deinitI2C()
{
    i2c_deinit(i2c0);
}

void scanSccbBus()
{
    printf("Scanning SCCB/I2C bus...\n");
    for (uint8_t address = 0x08; address <= 0x77; ++address)
    {
        uint8_t dummy = 0;
        if (i2c_read_timeout_us(i2c0, address, &dummy, 1, false, I2CTimeoutUS) >= 0)
        {
            printf("  Device responded at 0x%02X\n", address);
        }
    }
}

int main()
{
    stdio_init_all();
    sleep_ms(2000);
    printf("OV5640 SCCB example starting...\n");

    initCameraClock();

    if (!initI2C())
    {
        printf("Failed to initialize SCCB/I2C interface.\n");
        deinitI2C();
        while (true)
        {
            tight_loop_contents();
        }
    }

    jsi::SCCB camera_bus(writeI2C, readI2C);

    scanSccbBus();

    uint8_t active_address = 0;
    uint8_t chip_id[2]     = {0, 0};
    bool    camera_found   = false;

    for (const uint8_t candidate : Ov5640CandidateAddresses)
    {
        if (camera_bus.readRegister(candidate, OV5640ChipIDRegister, chip_id, 2))
        {
            active_address = candidate;
            camera_found   = true;
            break;
        }
    }

    if (!camera_found)
    {
        printf("Failed to read OV5640 identification registers on 0x3C/0x3D.\n");
        deinitI2C();
        while (true)
        {
            tight_loop_contents();
        }
    }

    printf("OV5640 detected at 0x%02X ID: 0x%02X 0x%02X\n", active_address, chip_id[0], chip_id[1]);

    uint8_t system_control = 0;
    if (camera_bus.readRegister(active_address, OV5640SYSCTRLRegister, system_control))
    {
        printf("System control register (0x3008): 0x%02X\n", system_control);

        if (camera_bus.writeRegister(active_address, OV5640SYSCTRLRegister, system_control))
        {
            printf("Wrote 0x%02X back to system control register (demonstration write).\n", system_control);
        }
        else
        {
            printf("Failed to write system control register.\n");
        }
    }
    else
    {
        printf("Failed to read system control register.\n");
    }

    deinitI2C();

    while (true)
    {
        tight_loop_contents();
    }
}
