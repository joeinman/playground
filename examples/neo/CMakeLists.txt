cmake_minimum_required(VERSION 3.22)

project(neo_example)

add_executable(${PROJECT_NAME}
  "src/${PROJECT_NAME}.cpp"
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    "include"
)

target_link_libraries(${PROJECT_NAME}
  pico_stdlib
  hardware_i2c
  rpixel
  neo
)

# Enable USB Output
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)
