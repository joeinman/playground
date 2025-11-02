cmake_minimum_required(VERSION 3.22)

project(ov5640)

# Add library
add_library(ov5640 STATIC
    src/ov5640.cpp
)

# Generate PIO header
pico_generate_pio_header(ov5640 ${CMAKE_CURRENT_LIST_DIR}/src/camera_capture.pio)

# Include directories
target_include_directories(ov5640 PUBLIC
    include
    src
)

# Link libraries
target_link_libraries(ov5640 PUBLIC
    pico_stdlib
    hardware_pio
    hardware_dma
    hardware_gpio
    sccb
)
