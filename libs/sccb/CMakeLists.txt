cmake_minimum_required(VERSION 3.22)

project(sccb)

add_library(sccb STATIC
    "src/sccb.cpp"
)

target_include_directories(sccb
    PUBLIC
        "include"
        "src"
)
