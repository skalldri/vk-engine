cmake_minimum_required(VERSION 3.21)

project(vk-engine CXX)

add_executable(engine)

add_subdirectory(ext)

target_sources(
    engine
    PRIVATE
        src/main.cpp
)

target_link_libraries(
    engine
    PRIVATE
        fmt::fmt
        CLI11::CLI11
)