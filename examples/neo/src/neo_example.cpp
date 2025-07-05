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

    auto notification_bar = std::make_shared<NotificationBar>();
    neo->loadScene(notification_bar);

    while (true)
    {
        auto c = getchar_timeout_us(0);
        if (c != PICO_ERROR_TIMEOUT)
        {
            if (c == '1')
            {
                notification_bar->pushEvent(Event<NotificationBarEventType>{NotificationBarEventType::kSetModeOff,
                                                                            time_us_64(),
                                                                            time_us_64() + 1000000});
            }
            else if (c == '2')
            {
                notification_bar->pushEvent(Event<NotificationBarEventType>{NotificationBarEventType::kSetModeBootingUp,
                                                                            time_us_64(),
                                                                            time_us_64() + 1000000});
            }
            else if (c == '3')
            {
                notification_bar->pushEvent(Event<NotificationBarEventType>{NotificationBarEventType::kSetModeCharging,
                                                                            time_us_64(),
                                                                            time_us_64() + 1000000});
            }
            else if (c == '4')
            {
                notification_bar->pushEvent(
                    Event<NotificationBarEventType>{NotificationBarEventType::kSetModeOperational,
                                                    time_us_64(),
                                                    time_us_64() + 1000000});
            }
            else if (c == '5')
            {
                notification_bar->pushEvent(
                    Event<NotificationBarEventType>{NotificationBarEventType::kSetModeBootingDown,
                                                    time_us_64(),
                                                    time_us_64() + 1000000});
            }
            else if (c == '6')
            {
                notification_bar->pushEvent(
                    Event<NotificationBarEventType>{NotificationBarEventType::kSetModeAutonomous,
                                                    time_us_64(),
                                                    time_us_64() + 1000000});
            }
            else
            {
                printf("Unknown command: %c\n", c);
            }
        }

        notification_bar->spin();
        neo->spin();
    }
}
