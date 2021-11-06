cmake_minimum_required(VERSION 3.21)

project(vk-engine CXX)

add_executable(engine)

target_sources(
    engine
    PRIVATE
        main.cpp
)