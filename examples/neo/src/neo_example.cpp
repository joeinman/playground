#include <pico/stdlib.h>
#include <stdio.h>

#include <rpixel/rpixel.hpp>
#include <neo/neo.hpp>
#include <neo/component/rectangle.hpp>

#include <math.h>

constexpr uint8_t LEDDataPin = 4;
constexpr uint8_t LEDCount   = 30;
constexpr bool    IsRGBW     = false;

constexpr uint64_t IndicatorWidth = 5;
constexpr uint64_t BarWidth       = 8;
constexpr uint64_t rangeX         = (LEDCount - 2 * IndicatorWidth) - BarWidth + 1;

using namespace jsi::neo;

float get_sin(float frequency_hz)
{
    float time_sec = static_cast<float>(time_us_64()) * 1e-6f;
    float raw      = sinf(2.0f * static_cast<float>(M_PI) * frequency_hz * time_sec);
    return 0.5f * (raw + 1.0f);
}

int main()
{
    stdio_init_all();

    jsi::RPixel led_strip(LEDDataPin, LEDCount, IsRGBW);
    auto        neo = std::make_shared<Neo>(
        ScreenSize(LEDCount, 1),
        [&led_strip](size_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t /*w*/) {
            led_strip.setPixel(index, r, g, b);
        },
        [&led_strip]() { led_strip.show(); },
        time_us_64);

    auto scene = std::make_shared<Scene>();
    scene->addComponent<Rectangle>(0, 0, IndicatorWidth, 1, Color(255, 255, 150));
    scene->addComponent<Rectangle>(LEDCount - IndicatorWidth, 0, IndicatorWidth, 1, Color(255, 255, 150));
    auto bar_id = scene->addComponent<Rectangle>(0, 0, BarWidth, 1, Color(255, 255, 0, 180));
    neo->loadScene(scene);

    while (true)
    {
        uint8_t x = IndicatorWidth + (uint8_t) (get_sin(1) * rangeX);
        scene->setComponentProperty<uint8_t>(bar_id, "x", x);

        neo->spin();
    }
}
