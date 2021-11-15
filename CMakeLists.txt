cmake_minimum_required(VERSION 3.21)

project(vk-engine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED true)

find_package(Vulkan REQUIRED COMPONENTS glslc)
find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)

add_subdirectory(ext)
add_subdirectory(src)

add_executable(engine)

target_sources(
    engine
    PRIVATE
        src/main.cpp
        src/engine/utils/to_string.cpp
)

target_include_directories(
    engine
    PRIVATE
        src
)

target_compile_features(engine PUBLIC cxx_std_17)

target_link_libraries(
    engine
    PRIVATE
        glm
        glfw
        fmt::fmt
        CLI11::CLI11
        Vulkan::Vulkan
)

add_dependencies(engine shaders)

# Works correctly for both single and multi-config generators
target_compile_definitions(engine PRIVATE
  $<$<CONFIG:Debug>:DEBUG_BUILD>
)

target_compile_options(engine PUBLIC /EHsc /Zi)
target_link_options(engine PUBLIC /DEBUG:FULL)
