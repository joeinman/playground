#include <pico/stdlib.h>
#include <stdio.h>

#include <rpixel/rpixel.hpp>
#include <neo/neo.hpp>
#include <neo/component/rectangle.hpp>
#include <neo/component/waveform_generator.hpp>

constexpr uint8_t LEDDataPin = 4;
constexpr uint8_t LEDCount   = 30;
constexpr bool    IsRGBW     = false;

using namespace jsi::neo;

class NotificationBarScene : public Scene
{
    const uint8_t IndicatorWidth = 5;
    const uint8_t BarWidth       = 8;
    const uint8_t rangeX         = static_cast<uint8_t>((LEDCount - 2 * IndicatorWidth) - BarWidth + 1);

public:
    NotificationBarScene()
    {
        addComponent<Rectangle>(0, 0, IndicatorWidth, 1, Color(255, 255, 255));
        addComponent<Rectangle>(LEDCount - IndicatorWidth, 0, IndicatorWidth, 1, Color(255, 255, 255));
        addComponent<Rectangle>(0, 0, BarWidth, 1, Color(255, 255, 0));
        addComponent<WaveformGenerator>(1.0, WaveformType::kTriangle);
    }

    void tick(uint64_t time_us) override
    {
        Scene::tick(time_us);

        auto    waveform_value = getComponent(3)->getOutput<double>("waveform_value").value();
        uint8_t x              = IndicatorWidth + static_cast<uint8_t>(waveform_value * rangeX);
        setComponentProperty<uint8_t>(2, "x", x);
    }
};

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

    auto scene = std::make_shared<NotificationBarScene>();
    neo->loadScene(scene);

    while (true)
    {
        neo->spin();
    }
}
