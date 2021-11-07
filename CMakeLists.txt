cmake_minimum_required(VERSION 3.21)

project(vk-engine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED true)

add_subdirectory(ext)

add_library(vulkan STATIC IMPORTED)
set_target_properties(vulkan PROPERTIES IMPORTED_LOCATION C:/VulkanSDK/1.2.189.2/Lib/vulkan-1.lib IMPORTED_IMPLIB C:/VulkanSDK/1.2.189.2/Lib/vulkan-1.lib)
target_include_directories(vulkan INTERFACE C:/VulkanSDK/1.2.189.2/Include)

add_executable(engine)

target_sources(
    engine
    PRIVATE
        src/main.cpp
)

target_compile_features(engine PUBLIC cxx_std_17)

target_link_libraries(
    engine
    PRIVATE
        glm
        glfw
        fmt::fmt
        CLI11::CLI11
        vulkan
)

# Works correctly for both single and multi-config generators
target_compile_definitions(engine PRIVATE
  $<$<CONFIG:Debug>:DEBUG_BUILD>
)
