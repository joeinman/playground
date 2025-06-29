#include <pico/stdlib.h>
#include <stdio.h>

#include <rpixel/rpixel.hpp>
#include <neo/neo.hpp>
#include <neo/component/rectangle.hpp>

constexpr uint8_t LEDDataPin = 4;
constexpr uint8_t LEDCount   = 30;
constexpr bool    IsRGBW     = false;

using namespace jsi::neo;

int main()
{
    stdio_init_all();

    jsi::RPixel led_strip(LEDDataPin, LEDCount, IsRGBW);
    auto        neo = std::make_shared<Neo>(
        ScreenSize(LEDCount, 1),
        [&led_strip](size_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
            led_strip.setPixel(index, r, g, b, w);
        },
        [&led_strip]() { led_strip.show(); },
        time_us_64);

    auto scene = std::make_shared<Scene>();
    scene->addComponent<Rectangle>(0, 0, PortName("width"), 1, Color(255, 0, 0));
    neo->loadScene(scene);

    while (true)
    {
        neo->spin();
    }
}
