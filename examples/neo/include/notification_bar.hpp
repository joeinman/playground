#pragma once

#include <neo/neo.hpp>
#include <neo/component/rectangle.hpp>
#include <neo/component/waveform_generator.hpp>
#include <neo/component/algebra_unit.hpp>

#include "software_definitions.hpp"

using namespace jsi::neo;

class NotificationBar : public Scene
{
public:
    NotificationBar()
    {
        addComponent<Rectangle>("left_indicator", 0, 0, IndicatorWidth, 1, Color(255, 255, 255), 1);
        addComponent<Rectangle>("right_indicator",
                                LEDCount - IndicatorWidth,
                                0,
                                IndicatorWidth,
                                1,
                                Color(255, 255, 255),
                                1);
        addComponent<Rectangle>("moving_bar", 0, 0, BarWidth, 1, Color(255, 0, 0));
        addComponent<WaveformGenerator>("waveform_generator", 1.0, WaveformType::kTriangle);

        addComponent<AlgebraUnit<int16_t>>("width_calculator", [](const PortList& properties) {
            auto left_indicator_position  = properties.get<int16_t>("left_indicator_position").value_or(0);
            auto right_indicator_position = properties.get<int16_t>("right_indicator_position").value_or(0);

            return (right_indicator_position - left_indicator_position - 2 * IndicatorWidth) - 1;
        });
        connectProperties<int16_t>("left_indicator", "x", "width_calculator", "left_indicator_position");
        connectProperties<int16_t>("right_indicator", "x", "width_calculator", "right_indicator_position");

        addComponent<AlgebraUnit<int16_t>>("position_calculator", [](const PortList& properties) {
            auto left_indicator_position = properties.get<int16_t>("left_indicator_position").value_or(0);
            auto width_calculation       = properties.get<int16_t>("width_calculation").value_or(0);
            auto waveform_input          = properties.get<double>("waveform_input").value_or(0.0);

            return (left_indicator_position + IndicatorWidth) + (width_calculation * waveform_input);
        });
        connectProperties<int16_t>("left_indicator", "x", "position_calculator", "left_indicator_position");
        connectProperties<int16_t>("width_calculator", "output_value", "position_calculator", "width_calculation");
        connectProperties<double>("waveform_generator", "output_value", "position_calculator", "waveform_input");
        connectProperties<int16_t>("position_calculator", "output_value", "moving_bar", "x");

        addComponent<AlgebraUnit<double>>("frequency_calculator", [](const PortList& properties) {
            auto width_calculation = properties.get<int16_t>("width_calculation").value_or(0);

            return 10 / static_cast<double>(width_calculation + 1);
        });
        connectProperties<int16_t>("width_calculator", "output_value", "frequency_calculator", "width_calculation");
        connectProperties<double>("frequency_calculator", "output_value", "waveform_generator", "frequency");
    }

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
