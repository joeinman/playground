cmake_minimum_required(VERSION 3.22)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialize the Raspberry Pi Pico SDK
include(libs/pico-sdk/pico_sdk_init.cmake)

project(playground)

set(PICO_CXX_ENABLE_RTTI 1)
set(PICO_CXX_ENABLE_EXCEPTIONS 0)
pico_sdk_init()

# Set the C/C++ standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)

# Format ninja output
option (FORCE_COLORED_OUTPUT "Always produce ANSI-colored output (GNU/Clang only)." TRUE)
if (${FORCE_COLORED_OUTPUT})
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
       add_compile_options (-fdiagnostics-color=always)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
       add_compile_options (-fcolor-diagnostics)
    endif ()
endif ()

# Add Examples
add_subdirectory(examples)

# Add Libraries
add_subdirectory(libs)
