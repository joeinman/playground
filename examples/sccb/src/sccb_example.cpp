#include <hardware/clocks.h>
#include <hardware/gpio.h>
#include <hardware/i2c.h>
#include <hardware/pwm.h>
#include <pico/stdlib.h>

#include <array>
#include <cstdio>

#include <sccb/sccb.hpp>

namespace
{

constexpr uint     I2cFrequencyHz  = 100 * 1000;
constexpr uint     I2cSdaPin       = 8;
constexpr uint     I2cSclPin       = 9;
constexpr uint32_t I2cTimeoutUs    = 1'000;
constexpr uint     XclkPin         = 5;
constexpr uint32_t XclkFrequencyHz = 24'000'000;
constexpr uint16_t XclkWrap        = 1;
constexpr uint16_t RegisterChipId  = 0x300A;
constexpr uint16_t RegisterSysCtrl = 0x3008;

constexpr std::array<uint8_t, 2> Ov5640CandidateAddresses = {0x3C, 0x3D};

// Set either constant to -1 if your module hardwires the signal and you do not
// have it connected to the Pico.
constexpr int CameraPwdnPin  = 2;
constexpr int CameraResetPin = 3;

void initCameraClock()
{
    const uint slice = pwm_gpio_to_slice_num(XclkPin);

    gpio_set_function(XclkPin, GPIO_FUNC_PWM);

    pwm_config  config  = pwm_get_default_config();
    const float clk_div = static_cast<float>(clock_get_hz(clk_sys)) /
                          (static_cast<float>(XclkFrequencyHz) * static_cast<float>(XclkWrap + 1));

    pwm_config_set_clkdiv(&config, clk_div);
    pwm_config_set_wrap(&config, XclkWrap);
    pwm_init(slice, &config, false);

    const uint16_t duty_level = (XclkWrap + 1) / 2;
    pwm_set_gpio_level(XclkPin, duty_level);
    pwm_set_enabled(slice, true);
    sleep_ms(5);
}

bool initI2C()
{
    i2c_init(i2c0, I2cFrequencyHz);
    gpio_set_function(I2cSdaPin, GPIO_FUNC_I2C);
    gpio_set_function(I2cSclPin, GPIO_FUNC_I2C);
    gpio_pull_up(I2cSdaPin);
    gpio_pull_up(I2cSclPin);
    sleep_ms(10);
    return true;
}

void initCameraControlPins()
{
    if (CameraPwdnPin >= 0)
    {
        const uint pin = static_cast<uint>(CameraPwdnPin);
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_OUT);
        gpio_put(pin, 1);  // hold power down active
    }

    if (CameraResetPin >= 0)
    {
        const uint pin = static_cast<uint>(CameraResetPin);
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_OUT);
        gpio_put(pin, 0);  // assert reset
    }

    sleep_ms(5);

    if (CameraPwdnPin >= 0)
    {
        gpio_put(static_cast<uint>(CameraPwdnPin), 0);  // release power down
    }

    sleep_ms(5);

    if (CameraResetPin >= 0)
    {
        gpio_put(static_cast<uint>(CameraResetPin), 1);  // release reset
    }

    sleep_ms(20);
}

bool writeI2C(uint8_t device_addr, const uint8_t* data, size_t len, bool nostop)
{
    if (data == nullptr || len == 0)
    {
        return false;
    }

    const int result = i2c_write_timeout_us(i2c0, device_addr, data, len, nostop, I2cTimeoutUs);
    return result >= 0 && static_cast<size_t>(result) == len;
}

bool readI2C(uint8_t device_addr, uint8_t* data, size_t len)
{
    if (data == nullptr || len == 0)
    {
        return false;
    }

    const int result = i2c_read_timeout_us(i2c0, device_addr, data, len, false, I2cTimeoutUs);
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
        if (i2c_read_timeout_us(i2c0, address, &dummy, 1, false, I2cTimeoutUs) >= 0)
        {
            printf("  Device responded at 0x%02X\n", address);
        }
    }
}

}  // namespace

int main()
{
    stdio_init_all();
    sleep_ms(2000);  // allow USB CDC to enumerate

    printf("OV5640 SCCB example starting...\n");

    initCameraClock();
    initCameraControlPins();

    jsi::SCCB camera_bus(initI2C, writeI2C, readI2C, deinitI2C);
    if (!camera_bus.isInitialized())
    {
        printf("Failed to initialize SCCB interface.\n");
        while (true)
        {
            sleep_ms(1000);
        }
    }

    scanSccbBus();

    uint8_t active_address = 0;
    uint8_t chip_id[2]     = {0, 0};
    bool    camera_found   = false;

    for (const uint8_t candidate : Ov5640CandidateAddresses)
    {
        if (camera_bus.readRegister(candidate, RegisterChipId, chip_id, 2))
        {
            active_address = candidate;
            camera_found   = true;
            break;
        }
    }

    if (!camera_found)
    {
        printf("Failed to read OV5640 identification registers on 0x3C/0x3D.\n");
        printf("Verify wiring, power rails, and the RESET/PWDN control lines.\n");
        while (true)
        {
            sleep_ms(1000);
        }
    }

    printf("OV5640 detected at 0x%02X ID: 0x%02X 0x%02X\n", active_address, chip_id[0], chip_id[1]);

    uint8_t system_control = 0;
    if (camera_bus.readRegister(active_address, RegisterSysCtrl, system_control))
    {
        printf("System control register (0x3008): 0x%02X\n", system_control);

        if (camera_bus.writeRegister(active_address, RegisterSysCtrl, system_control))
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

    while (true)
    {
        sleep_ms(1000);
    }
}
