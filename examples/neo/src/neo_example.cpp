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
        time_us_64,
        1.5f);
    neo->setScreenBrightness(0.5);

    auto notification_bar = std::make_shared<NotificationBar>();
    neo->loadScene(notification_bar);

    while (true)
    {
        auto c = getchar_timeout_us(10);
        if (c != PICO_ERROR_TIMEOUT)
        {
            // If is a number, set the notification bar mode
            if (c >= '0' && c <= '9')
            {
                uint8_t mode = c - '0';
                if (mode < static_cast<uint8_t>(NotificationBarMode::kHardEStop) + 1)
                {
                    notification_bar->set_mode(static_cast<NotificationBarMode>(mode));
                }
            }
        }

        neo->spin();
    }
}
