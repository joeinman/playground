#include <pico/stdlib.h>
#include <stdio.h>

#include <rpixel/rpixel.hpp>
#include <neo/neo.hpp>

constexpr uint8_t LEDDataPin = 4;
constexpr uint8_t LEDCount   = 30;
constexpr bool    IsRGBW     = false;

using namespace jsi::neo;

class FlashingBarScene : public Scene
{
public:
    FlashingBarScene()
    {
        auto bar_id      = addComponent<Rectangle>(0, 0, LEDCount, 1, Color(255, 0, 0));
        auto waveform_id = addComponent<WaveformGenerator>(4.0, WaveformType::kSquare);

        connectComponentProperty<double, uint8_t>(waveform_id,
                                                  "output_value",
                                                  bar_id,
                                                  "a",
                                                  [](double output_value) -> uint8_t {
                                                      return static_cast<uint8_t>(output_value * 255);
                                                  });
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

    neo->loadScene(std::make_shared<FlashingBarScene>());

    while (true)
    {
        neo->spin();
    }
}
