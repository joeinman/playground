set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wshadow -Wnon-virtual-dtor -Wcast-align -Wunused -Woverloaded-virtual -Wconversion -Wsign-conversion -Wduplicated-cond -Wnull-dereference -Wuseless-cast -Wdouble-promotion -Wformat=2 -pedantic")
add_compile_definitions(PICO_XOSC_STARTUP_DELAY_MULTIPLIER=64)

add_subdirectory(rpixel)
add_subdirectory(neo)
