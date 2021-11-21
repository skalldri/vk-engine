
add_library(core STATIC)

target_sources(
    core
    PRIVATE
        ShaderModule.cpp)

target_include_directories(
    core
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/../../ # Expose the "engine" root folder
)

target_link_libraries(
    core
    PRIVATE
        utils
        fmt::fmt
        Vulkan::Vulkan
)