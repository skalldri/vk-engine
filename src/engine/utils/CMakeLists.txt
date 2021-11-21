add_library(utils STATIC)

target_sources(
    utils
    PRIVATE
        to_string.cpp
)

target_include_directories(
    utils
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(
    utils
    PRIVATE
        Vulkan::Vulkan
)