#pragma once

#include <neo/neo.hpp>
#include <neo/component/rectangle.hpp>
#include <neo/component/waveform_generator.hpp>
#include <neo/component/algebra_unit.hpp>

#include "software_definitions.hpp"
#include "events.hpp"
#include <pico/stdlib.h>

using namespace jsi::neo;

const Color color_white     = {255, 255, 255};
const Color color_yellow    = {255, 255, 0};
const Color color_red       = {255, 0, 0};
const Color color_green     = {0, 255, 0};
const Color color_aqua_blue = {12, 210, 245};

class NotificationBar : public Scene
{
public:
    NotificationBar()
    {
        event_queue_ = std::make_unique<EventQueue<NotificationBarEventType>>();

        addComponent<Rectangle>("left_indicator", 0, 0, IndicatorWidth, 1, Color(255, 255, 255, 0), 1);
        addComponent<Rectangle>("right_indicator",
                                LEDCount - IndicatorWidth,
                                0,
                                IndicatorWidth,
                                1,
                                Color(255, 255, 255, 0),
                                1);
        addComponent<Rectangle>("moving_bar",
                                0,
                                0,
                                BarWidth,
                                1,
                                Color(color_yellow.r_, color_yellow.g_, color_yellow.b_, 0));

        addComponent<Rectangle>("background", 0, 0, LEDCount, 1, Color(0, 255, 0, 0));
        addComponent<WaveformGenerator>("waveform_generator", 1.0, WaveformType::kTriangle);

        addComponent<AlgebraUnit<int16_t>>("position_calculator", [](const PortList& properties) {
            auto left_indicator_x  = properties.get<int16_t>("left_indicator_x").value_or(0);
            auto right_indicator_x = properties.get<int16_t>("right_indicator_x").value_or(0);
            auto waveform_input    = properties.get<double>("waveform_generator_output_value").value_or(0.0);

            auto res = (right_indicator_x - left_indicator_x - 2 * IndicatorWidth) - 1;
            return (left_indicator_x + IndicatorWidth) + (res * waveform_input);
        });
        connectProperties<int16_t>("left_indicator", "x", "position_calculator", "left_indicator_x");
        connectProperties<int16_t>("right_indicator", "x", "position_calculator", "right_indicator_x");
        connectProperties<double>("waveform_generator",
                                  "output_value",
                                  "position_calculator",
                                  "waveform_generator_output_value");
        connectProperties<int16_t>("position_calculator", "output_value", "moving_bar", "x");

        addComponent<WaveformGenerator>("background_alpha_generator", 0.4, WaveformType::kSine);
        addComponent<AlgebraUnit<uint8_t>>("background_alpha_calculator", [](const PortList& properties) {
            auto waveform_input = properties.get<double>("background_alpha_calculator_output_value").value_or(0.0);
            return static_cast<uint8_t>(250 * waveform_input);
        });
        connectProperties<double>("background_alpha_generator",
                                  "output_value",
                                  "background_alpha_calculator",
                                  "background_alpha_calculator_output_value");
        connectProperties<uint8_t>("background_alpha_calculator", "output_value", "background", "a");
        setComponentProperty<bool>("binding_background_alpha_calculator_output_value_background_a", "enabled", false);
    }

    void spin()
    {
        auto event = event_queue_->pop_front().value_or(Event<NotificationBarEventType>{});
        if (event.deadline_us != 0 && event.timestamp_us < time_us_64())
        {
            switch (event.type)
            {
            case NotificationBarEventType::kSetModeOff:
                handle_off_state();
                break;
            case NotificationBarEventType::kSetModeBootingDown:
                handle_booting_down_state();
                break;
            case NotificationBarEventType::kSetModeCharging:
                handle_charging_state();
                break;
            case NotificationBarEventType::kSetModeOperational:
                handle_operational_state();
                break;
            case NotificationBarEventType::kSetModeAutonomous:
                handle_autonomous_state();
                break;
            default:
                break;
            }
        }
    }

    void pushEvent(Event<NotificationBarEventType> event) { event_queue_->push(event); }

    void handle_off_state()
    {
        enable_indicators(false);
        setComponentProperty<uint8_t>("moving_bar", "a", 0, TransitionType::kLinear, 250000);

        setComponentProperty<bool>("binding_background_alpha_calculator_output_value_background_a", "enabled", false);

        setComponentProperty<uint8_t>("background", "a", 0, TransitionType::kLinear, 250000);
        setComponentProperty<uint8_t>("background", "r", 0, TransitionType::kLinear, 250000);
        setComponentProperty<uint8_t>("background", "g", 0, TransitionType::kLinear, 250000);
        setComponentProperty<uint8_t>("background", "b", 0, TransitionType::kLinear, 250000);
    }

    void handle_booting_down_state()
    {
        enable_indicators(true);
        setComponentProperty<uint8_t>("moving_bar", "a", 250, TransitionType::kLinear, 250000);
        setComponentProperty<double>("waveform_generator", "cycle_position", 0.0);

        setComponentProperty<bool>("binding_background_alpha_calculator_output_value_background_a", "enabled", false);
        setComponentProperty<uint8_t>("background", "a", 0, TransitionType::kLinear, 250000);
        setComponentProperty<uint8_t>("background", "r", 0, TransitionType::kLinear, 250000);
        setComponentProperty<uint8_t>("background", "g", 0, TransitionType::kLinear, 250000);
        setComponentProperty<uint8_t>("background", "b", 0, TransitionType::kLinear, 250000);
    }

    void handle_charging_state()
    {
        enable_indicators(true);
        setComponentProperty<uint8_t>("moving_bar", "a", 0, TransitionType::kLinear, 250000);

        // Set background color
        setComponentProperty<uint8_t>("background", "r", 0, TransitionType::kLinear, 250000);
        setComponentProperty<uint8_t>("background", "g", 250, TransitionType::kLinear, 250000);
        setComponentProperty<uint8_t>("background", "b", 0, TransitionType::kLinear, 250000);

        // Reset the waveform generator position
        auto   res = getComponentProperty<uint8_t>("background", "a").value();
        double cycle_position;

        if (res < 120)
        {
            cycle_position = 0.0;  // Start at the beginning of the cycle
        }
        else
        {
            cycle_position = 0.25;  // Midway through the cycle
        }
        setComponentProperty<double>("background_alpha_generator", "cycle_position", cycle_position);

        setComponentProperty<WaveformType>("background_alpha_generator", "waveform_type", WaveformType::kSine);
        setComponentProperty<double>("background_alpha_generator", "frequency", 0.4);
        setComponentProperty<bool>("binding_background_alpha_calculator_output_value_background_a", "enabled", true);
    }

    void handle_operational_state()
    {
        enable_indicators(true);
        setComponentProperty<bool>("binding_background_alpha_calculator_output_value_background_a", "enabled", false);
        setComponentProperty<uint8_t>("moving_bar", "a", 0, TransitionType::kLinear, 250000);
        setComponentProperty<uint8_t>("background", "a", 250, TransitionType::kLinear, 250000);
        setComponentProperty<uint8_t>("background", "r", 250, TransitionType::kLinear, 250000);
        setComponentProperty<uint8_t>("background", "g", 250, TransitionType::kLinear, 250000);
        setComponentProperty<uint8_t>("background", "b", 250, TransitionType::kLinear, 250000);
    }

    void handle_autonomous_state()
    {
        enable_indicators(true);
        setComponentProperty<uint8_t>("moving_bar", "a", 0, TransitionType::kLinear, 250000);

        setComponentProperty<double>("background_alpha_generator", "cycle_position", 0.0);
        setComponentProperty<WaveformType>("background_alpha_generator", "waveform_type", WaveformType::kSquare);
        setComponentProperty<double>("background_alpha_generator", "frequency", 3.0);
        setComponentProperty<bool>("binding_background_alpha_calculator_output_value_background_a", "enabled", true);

        setComponentProperty<uint8_t>("background", "a", 250, TransitionType::kLinear, 250000);
        setComponentProperty<uint8_t>("background", "r", color_aqua_blue.r_, TransitionType::kLinear, 250000);
        setComponentProperty<uint8_t>("background", "g", color_aqua_blue.g_, TransitionType::kLinear, 250000);
        setComponentProperty<uint8_t>("background", "b", color_aqua_blue.b_, TransitionType::kLinear, 250000);
    }

    void enable_indicators(bool enable)
    {
        auto left_indicator  = getComponent("left_indicator");
        auto right_indicator = getComponent("right_indicator");

        if (left_indicator && right_indicator)
        {
            if (!enable)
            {
                // Hide indicators by moving them off-screen and making them transparen
                left_indicator->setProperty<int16_t>("x", -IndicatorWidth, TransitionType::kLinear, 250000);
                left_indicator->setProperty<uint8_t>("a", 0, TransitionType::kLinear, 250000);

                right_indicator->setProperty<int16_t>("x", LEDCount, TransitionType::kLinear, 250000);
                right_indicator->setProperty<uint8_t>("a", 0, TransitionType::kLinear, 250000);
            }
            else
            {
                left_indicator->setProperty<uint8_t>("a", 250, TransitionType::kLinear, 250000);
                right_indicator->setProperty<uint8_t>("a", 250, TransitionType::kLinear, 250000);

                left_indicator->setProperty<int16_t>("x", 0, TransitionType::kLinear, 250000);
                right_indicator->setProperty<int16_t>("x",
                                                      LEDCount - IndicatorWidth + 1,
                                                      TransitionType::kLinear,
                                                      250000);
            }
        }
    }

private:
    std::unique_ptr<EventQueue<NotificationBarEventType>> event_queue_;
};
