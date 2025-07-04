#include <pico/stdlib.h>
#include <stdio.h>

#include <rpixel/rpixel.hpp>
#include <neo/neo.hpp>

constexpr uint8_t LEDDataPin = 4;
constexpr uint8_t LEDCount   = 30;
constexpr bool    IsRGBW     = false;

using namespace jsi::neo;

class NotificationBarScene : public Scene
{
    static inline const uint8_t IndicatorWidth = 5;
    static inline const uint8_t BarWidth       = 8;
    static inline const uint8_t XRange         = static_cast<uint8_t>((LEDCount - 2 * IndicatorWidth) - BarWidth + 1);

public:
    NotificationBarScene()
    {
        addComponent<Rectangle>(0, 0, IndicatorWidth, 1, Color(255, 255, 255));
        addComponent<Rectangle>(LEDCount - IndicatorWidth, 0, IndicatorWidth, 1, Color(255, 255, 255));
        auto bar_id                = addComponent<Rectangle>(0, 0, BarWidth, 1, Color(255, 255, 0));
        auto waveform_generator_id = addComponent<WaveformGenerator>(1.0, WaveformType::kTriangle);
        auto algebra_unit_id       = addComponent<AlgebraUnit<int16_t>>([](const PortList& properties) {
            return IndicatorWidth +
                   static_cast<int16_t>(properties.get<double>("waveform_input").value_or(0.0) * XRange);
        });

        connectComponentProperty<double>(waveform_generator_id, "output_value", algebra_unit_id, "waveform_input");
        connectComponentProperty<int16_t>(algebra_unit_id, "output_value", bar_id, "x");
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

    neo->loadScene(std::make_shared<NotificationBarScene>());

    while (true)
    {
        neo->spin();
    }
}
