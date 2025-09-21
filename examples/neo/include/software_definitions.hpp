#pragma once

#include <stdint.h>

constexpr uint8_t LEDDataPin = 4;
constexpr uint8_t LEDCount   = 30;
constexpr bool    IsRGBW     = false;

constexpr uint8_t IndicatorWidth = 5;
constexpr uint8_t BarWidth       = 8;
// constexpr uint8_t XRange         = static_cast<uint8_t>((LEDCount - 2 * IndicatorWidth) - BarWidth + 1);

enum class NotificationBarMode : uint8_t
{
    kOff = 0,
    kTeleoperation,
    kBootingUp,
    kSoftEStop,
    kGeneralError,
    kBaseStationConnected,  // Not Implemented
    kBaseStationCharging,
    kBaseStationCharged,
    kAutonomousInspecting,
    kAutonomousDriving,
    kAutonomousToBaseStation,
    kHardEStop,
    kBootingDown,
};
