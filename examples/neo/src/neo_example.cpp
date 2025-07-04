#include <pico/stdlib.h>
#include <stdio.h>

#include <rpixel/rpixel.hpp>
#include <neo/neo.hpp>

#include "notification_bar.hpp"
#include "software_definitions.hpp"

using namespace jsi::neo;

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

    auto notification_bar = std::make_shared<NotificationBar>();
    neo->loadScene(notification_bar);

    while (true)
    {
        auto c = getchar_timeout_us(0);
        if (c != PICO_ERROR_TIMEOUT)
        {
            if (c == 'e' || c == 'E')
            {
                notification_bar->enable_indicators(true);
            }
            else if (c == 'd' || c == 'D')
            {
                notification_bar->enable_indicators(false);
            }
        }

        neo->spin();
    }
}
