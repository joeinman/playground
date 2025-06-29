#include <pico/stdlib.h>
#include <stdio.h>

#include <rpixel/rpixel.hpp>

constexpr uint8_t LEDDataPin = 4;
constexpr uint8_t LEDCount   = 30;
constexpr bool    IsRGBW     = false;

int main()
{
    stdio_init_all();

    jsi::RPixel led_strip(LEDDataPin, LEDCount, IsRGBW);

    while (true)
    {
        for (size_t i = 0; i < LEDCount; i++)
        {
            led_strip.setPixel(i, 255, 0, 0);
        }
        led_strip.show();
        sleep_ms(1000);

        for (size_t i = 0; i < LEDCount; i++)
        {
            led_strip.setPixel(i, 0, 255, 0);
        }
        led_strip.show();
        sleep_ms(1000);

        for (size_t i = 0; i < LEDCount; i++)
        {
            led_strip.setPixel(i, 0, 0, 255);
        }
        led_strip.show();
        sleep_ms(1000);
    }
}
