cmake_minimum_required(VERSION 3.22)

project(ov5640_example)

# Add executable
add_executable(ov5640_example
    src/ov5640_example.cpp
)

# Enable USB output, disable UART
pico_enable_stdio_usb(ov5640_example 1)
pico_enable_stdio_uart(ov5640_example 0)

# Generate UF2 file
pico_add_extra_outputs(ov5640_example)

# Link libraries
target_link_libraries(ov5640_example PRIVATE
    pico_stdlib
    hardware_i2c
    hardware_pwm
    sccb
    ov5640
)
