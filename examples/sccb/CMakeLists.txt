cmake_minimum_required(VERSION 3.22)

project(sccb_example)

add_executable(${PROJECT_NAME}
    "src/${PROJECT_NAME}.cpp"
)

target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    hardware_i2c
    hardware_pwm
    sccb
)

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

