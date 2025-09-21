#pragma once

#include <neo/neo.hpp>
#include <neo/component/rectangle.hpp>
#include <neo/component/waveform_generator.hpp>
#include <neo/component/algebra_unit.hpp>

#include "software_definitions.hpp"
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

        addComponent<AlgebraUnit<int16_t>>("booting_up_position_calculator", [](const PortList& properties) {
            auto left_indicator_x  = properties.get<int16_t>("left_indicator_x").value_or(0);
            auto right_indicator_x = properties.get<int16_t>("right_indicator_x").value_or(0);
            auto waveform_input    = properties.get<double>("waveform_generator_output_value").value_or(0.0);

            return static_cast<int16_t>(((right_indicator_x - left_indicator_x + IndicatorWidth) + BarWidth) *
                                        waveform_input) -
                   BarWidth;
        });
        connectProperties<int16_t>("left_indicator", "x", "booting_up_position_calculator", "left_indicator_x");
        connectProperties<int16_t>("right_indicator", "x", "booting_up_position_calculator", "right_indicator_x");
        connectProperties<double>("waveform_generator",
                                  "output_value",
                                  "booting_up_position_calculator",
                                  "waveform_generator_output_value");
        connectProperties<int16_t>("booting_up_position_calculator", "output_value", "moving_bar", "x");

        addComponent<AlgebraUnit<int16_t>>("booting_down_position_calculator", [](const PortList& properties) {
            auto left_indicator_x  = properties.get<int16_t>("left_indicator_x").value_or(0);
            auto right_indicator_x = properties.get<int16_t>("right_indicator_x").value_or(0);
            auto waveform_input    = properties.get<double>("waveform_generator_output_value").value_or(0.0);

            double normalized_input = 2.0 * waveform_input - 1.0;
            double center_point     = LEDCount / 2 - 1;
            double width =
                static_cast<double>(right_indicator_x - left_indicator_x - 1 - 2 * static_cast<double>(IndicatorWidth));

            return static_cast<int16_t>(center_point + (normalized_input * (width / 2)) - (IndicatorWidth / 2));
        });
        connectProperties<int16_t>("left_indicator", "x", "booting_down_position_calculator", "left_indicator_x");
        connectProperties<int16_t>("right_indicator", "x", "booting_down_position_calculator", "right_indicator_x");
        connectProperties<double>("waveform_generator",
                                  "output_value",
                                  "booting_down_position_calculator",
                                  "waveform_generator_output_value");
        connectProperties<int16_t>("booting_down_position_calculator", "output_value", "moving_bar", "x");

        addComponent<WaveformGenerator>("background_alpha_generator", 0.4, WaveformType::kSine);
        addComponent<AlgebraUnit<uint8_t>>("background_alpha_calculator", [](const PortList& properties) {
            auto   waveform_input   = properties.get<double>("background_alpha_calculator_output_value").value_or(0.0);
            double normalized_input = 2.0 * waveform_input - 1.0;
            return static_cast<uint8_t>(125 - (normalized_input * 125));
        });
        connectProperties<double>("background_alpha_generator",
                                  "output_value",
                                  "background_alpha_calculator",
                                  "background_alpha_calculator_output_value");
        connectProperties<uint8_t>("background_alpha_calculator", "output_value", "background", "a");
    }

    void set_mode(NotificationBarMode mode)
    {
        switch (mode)
        {
        case NotificationBarMode::kOff:
            printf("NotificationBar: Setting mode to Off\n");
            handle_off_state();
            break;
        case NotificationBarMode::kTeleoperation:
            printf("NotificationBar: Setting mode to Teleoperation\n");
            handle_teleoperation_state();
            break;
        case NotificationBarMode::kSoftEStop:
            printf("NotificationBar: Setting mode to Soft E-Stop\n");
            handle_soft_e_stop_state();
            break;
        // case NotificationBarMode::kBootingUp:
        //     handle_booting_up_state();
        //     break;
        // case NotificationBarMode::kGeneralError:
        //     handle_general_error_state();
        //     break;
        // case NotificationBarMode::kBaseStationCharging:
        //     handle_base_station_charging_state();
        //     break;
        // case NotificationBarMode::kBaseStationCharged:
        //     handle_base_station_charged_state();
        //     break;
        // case NotificationBarMode::kAutonomousInspecting:
        //     handle_autonomous_inspecting_state();
        //     break;
        // case NotificationBarMode::kAutonomousDriving:
        //     handle_autonomous_driving_state();
        //     break;
        // case NotificationBarMode::kAutonomousToBaseStation:
        //     handle_autonomous_to_base_station_state();
        //     break;
        // case NotificationBarMode::kHardEStop:
        //     handle_hard_e_stop_state();
        //     break;
        // case NotificationBarMode::kBootingDown:
        //     handle_booting_down_state();
        //     break;
        default:
            break;
        }
    }

    void set_background_color(const Color& color)
    {
        setComponentProperty<uint8_t>("background", "r", color.r_, TransitionType::kLinear, 250000);
        setComponentProperty<uint8_t>("background", "g", color.g_, TransitionType::kLinear, 250000);
        setComponentProperty<uint8_t>("background", "b", color.b_, TransitionType::kLinear, 250000);
        setComponentProperty<uint8_t>("background", "a", color.a_, TransitionType::kLinear, 250000);
    }

    void set_moving_bar_color(const Color& color)
    {
        setComponentProperty<uint8_t>("moving_bar", "r", color.r_, TransitionType::kLinear, 250000);
        setComponentProperty<uint8_t>("moving_bar", "g", color.g_, TransitionType::kLinear, 250000);
        setComponentProperty<uint8_t>("moving_bar", "b", color.b_, TransitionType::kLinear, 250000);
        setComponentProperty<uint8_t>("moving_bar", "a", color.a_, TransitionType::kLinear, 250000);
    }

    void handle_off_state()
    {
        enable_indicators(false);poweroff

        setComponentProperty<bool>("binding_background_alpha_generator_output_value_background_a", "enabled", false);

        set_moving_bar_color(Color(0, 0, 0, 0));
        set_background_color(Color(0, 0, 0, 0));
    }

    void handle_teleoperation_state()
    {
        enable_indicators(true);

        setComponentProperty<bool>("binding_background_alpha_generator_output_value_background_a", "enabled", false);

        set_moving_bar_color(Color(0, 0, 0, 0));
        set_background_color(color_white);
    }

    void handle_soft_e_stop_state()
    {
        enable_indicators(true);
        set_moving_bar_color(Color(0, 0, 0, 0));

        setComponentProperty<double>("background_alpha_generator", "cycle_position", 0.0);
        setComponentProperty<WaveformType>("background_alpha_generator", "waveform_type", WaveformType::kSquare);
        setComponentProperty<double>("background_alpha_generator", "frequency", 3.0);

        set_background_color(color_red);
    }

    // void handle_booting_up_state()
    // {
    //     enable_indicators(true);
    //     setComponentProperty<uint8_t>("moving_bar", "a", 250, TransitionType::kLinear, 250000);

    //     setComponentProperty<bool>("binding_booting_down_position_calculator_output_value_moving_bar_x",
    //                                "enabled",
    //                                false);
    //     setComponentProperty<bool>("binding_booting_up_position_calculator_output_value_moving_bar_x", "enabled",
    //     true);

    //     setComponentProperty<WaveformType>("waveform_generator", "waveform_type", WaveformType::kSawtooth);
    //     setComponentProperty<double>("waveform_generator", "cycle_position", 0.00);
    //     setComponentProperty<double>("waveform_generator", "frequency", 0.75);

    //     setComponentProperty<uint8_t>("background", "a", 0, TransitionType::kLinear, 250000);
    //     setComponentProperty<uint8_t>("background", "r", 0, TransitionType::kLinear, 250000);
    //     setComponentProperty<uint8_t>("background", "g", 0, TransitionType::kLinear, 250000);
    //     setComponentProperty<uint8_t>("background", "b", 0, TransitionType::kLinear, 250000);
    // }

    // void handle_general_error_state()
    // {
    //     enable_indicators(true);
    //     setComponentProperty<uint8_t>("moving_bar", "a", 0, TransitionType::kLinear, 250000);

    //     setComponentProperty<double>("background_alpha_generator", "cycle_position", 0.0);
    //     setComponentProperty<WaveformType>("background_alpha_generator", "waveform_type", WaveformType::kSquare);
    //     setComponentProperty<double>("background_alpha_generator", "frequency", 3.0);

    //     setComponentProperty<uint8_t>("background", "a", 250, TransitionType::kLinear, 250000);
    //     setComponentProperty<uint8_t>("background", "r", color_yellow.r_, TransitionType::kLinear, 250000);
    //     setComponentProperty<uint8_t>("background", "g", color_yellow.g_, TransitionType::kLinear, 250000);
    //     setComponentProperty<uint8_t>("background", "b", color_yellow.b_, TransitionType::kLinear, 250000);
    // }

    // void handle_base_station_charging_state()
    // {
    //     enable_indicators(true);
    //     setComponentProperty<uint8_t>("moving_bar", "a", 0, TransitionType::kLinear, 250000);

    //     // Set background color
    //     setComponentProperty<uint8_t>("background", "r", 0, TransitionType::kLinear, 250000);
    //     setComponentProperty<uint8_t>("background", "g", 250, TransitionType::kLinear, 250000);
    //     setComponentProperty<uint8_t>("background", "b", 0, TransitionType::kLinear, 250000);

    //     // Reset the waveform generator position;
    //     double cycle_position = getComponentProperty<uint8_t>("background", "a").value() < 120 ? 0.5 : 0.7;
    //     setComponentProperty<double>("background_alpha_generator", "cycle_position", cycle_position);

    //     setComponentProperty<WaveformType>("background_alpha_generator", "waveform_type", WaveformType::kSine);
    //     setComponentProperty<double>("background_alpha_generator", "frequency", 0.4);
    // }

    // void handle_base_station_charged_state()
    // {
    //     enable_indicators(true);
    //     setComponentProperty<uint8_t>("moving_bar", "a", 0, TransitionType::kLinear, 250000);

    //     setComponentProperty<uint8_t>("background", "r", 0, TransitionType::kLinear, 250000);
    //     setComponentProperty<uint8_t>("background", "g", 250, TransitionType::kLinear, 250000);
    //     setComponentProperty<uint8_t>("background", "b", 0, TransitionType::kLinear, 250000);
    // }

    // void handle_autonomous_inspecting_state()
    // {
    //     enable_indicators(true);
    //     setComponentProperty<uint8_t>("moving_bar", "r", color_aqua_blue.r_, TransitionType::kLinear, 250000);
    //     setComponentProperty<uint8_t>("moving_bar", "g", color_aqua_blue.g_, TransitionType::kLinear, 250000);
    //     setComponentProperty<uint8_t>("moving_bar", "b", color_aqua_blue.b_, TransitionType::kLinear, 250000);
    //     setComponentProperty<uint8_t>("moving_bar", "a", 250, TransitionType::kLinear, 250000);
    //     setComponentProperty<double>("waveform_generator", "cycle_position", 0.0);
    //     setComponentProperty<double>("waveform_generator", "frequency", 1.0);
    //     setComponentProperty<WaveformType>("waveform_generator", "waveform_type", WaveformType::kSquare);

    //     setComponentProperty<bool>("binding_booting_down_position_calculator_output_value_moving_bar_x",
    //                                "enabled",
    //                                true);
    //     setComponentProperty<bool>("binding_booting_up_position_calculator_output_value_moving_bar_x",
    //                                "enabled",
    //                                false);

    //     setComponentProperty<uint8_t>("background", "a", 0, TransitionType::kLinear, 250000);
    //     setComponentProperty<uint8_t>("background", "r", 0, TransitionType::kLinear, 250000);
    //     setComponentProperty<uint8_t>("background", "g", 0, TransitionType::kLinear, 250000);
    //     setComponentProperty<uint8_t>("background", "b", 0, TransitionType::kLinear, 250000);
    // }

    // void handle_autonomous_driving_state()
    // {
    //     enable_indicators(true);

    //     setComponentProperty<uint8_t>("moving_bar", "a", 0, TransitionType::kLinear, 250000);
    //     setComponentProperty<uint8_t>("background", "a", 250, TransitionType::kLinear, 250000);
    //     setComponentProperty<uint8_t>("background", "r", color_aqua_blue.r_, TransitionType::kLinear, 250000);
    //     setComponentProperty<uint8_t>("background", "g", color_aqua_blue.g_, TransitionType::kLinear, 250000);
    //     setComponentProperty<uint8_t>("background", "b", color_aqua_blue.b_, TransitionType::kLinear, 250000);
    // }

    // void handle_autonomous_to_base_station_state()
    // {
    //     enable_indicators(true);
    //     setComponentProperty<uint8_t>("moving_bar", "a", 0, TransitionType::kLinear, 250000);

    //     setComponentProperty<double>("background_alpha_generator", "cycle_position", 0.0);
    //     setComponentProperty<WaveformType>("background_alpha_generator", "waveform_type", WaveformType::kSquare);
    //     setComponentProperty<double>("background_alpha_generator", "frequency", 3.0);

    //     setComponentProperty<uint8_t>("background", "a", 250, TransitionType::kLinear, 250000);
    //     setComponentProperty<uint8_t>("background", "r", color_aqua_blue.r_, TransitionType::kLinear, 250000);
    //     setComponentProperty<uint8_t>("background", "g", color_aqua_blue.g_, TransitionType::kLinear, 250000);
    //     setComponentProperty<uint8_t>("background", "b", color_aqua_blue.b_, TransitionType::kLinear, 250000);
    // }

    // void handle_hard_e_stop_state()
    // {
    //     enable_indicators(false);
    //     setComponentProperty<uint8_t>("moving_bar", "a", 0, TransitionType::kLinear, 250000);

    //     setComponentProperty<double>("background_alpha_generator", "cycle_position", 0.0);
    //     setComponentProperty<WaveformType>("background_alpha_generator", "waveform_type", WaveformType::kSquare);
    //     setComponentProperty<double>("background_alpha_generator", "frequency", 3.0);

    //     setComponentProperty<uint8_t>("background", "a", 250, TransitionType::kLinear, 250000);
    //     setComponentProperty<uint8_t>("background", "r", color_red.r_, TransitionType::kLinear, 250000);
    //     setComponentProperty<uint8_t>("background", "g", color_red.g_, TransitionType::kLinear, 250000);
    //     setComponentProperty<uint8_t>("background", "b", color_red.b_, TransitionType::kLinear, 250000);
    // }

    // void handle_booting_down_state()
    // {
    //     enable_indicators(true);
    //     setComponentProperty<uint8_t>("moving_bar", "r", color_yellow.r_, TransitionType::kLinear, 250000);
    //     setComponentProperty<uint8_t>("moving_bar", "g", color_yellow.g_, TransitionType::kLinear, 250000);
    //     setComponentProperty<uint8_t>("moving_bar", "b", color_yellow.b_, TransitionType::kLinear, 250000);
    //     setComponentProperty<uint8_t>("moving_bar", "a", 250, TransitionType::kLinear, 250000);
    //     setComponentProperty<double>("waveform_generator", "cycle_position", 0.0);
    //     setComponentProperty<double>("waveform_generator", "frequency", 1.0);
    //     setComponentProperty<WaveformType>("waveform_generator", "waveform_type", WaveformType::kTriangle);

    //     setComponentProperty<bool>("binding_booting_down_position_calculator_output_value_moving_bar_x",
    //                                "enabled",
    //                                true);
    //     setComponentProperty<bool>("binding_booting_up_position_calculator_output_value_moving_bar_x",
    //                                "enabled",
    //                                false);

    //     setComponentProperty<uint8_t>("background", "a", 0, TransitionType::kLinear, 250000);
    //     setComponentProperty<uint8_t>("background", "r", 0, TransitionType::kLinear, 250000);
    //     setComponentProperty<uint8_t>("background", "g", 0, TransitionType::kLinear, 250000);
    //     setComponentProperty<uint8_t>("background", "b", 0, TransitionType::kLinear, 250000);
    // }

    void enable_indicators(bool enable)
    {
        auto left_indicator  = getComponent("left_indicator");
        auto right_indicator = getComponent("right_indicator");

        if (left_indicator && right_indicator)
        {
            if (!enable)
            {
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
};
