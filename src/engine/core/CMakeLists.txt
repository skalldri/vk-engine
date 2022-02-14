
add_library(core STATIC)

target_sources(
    core
    PRIVATE
        CommandPool.cpp
        Device.cpp
        GraphicsPipeline.cpp
        Image.cpp
        Instance.cpp
        ShaderModule.cpp
        Swapchain.cpp
        RenderPass.cpp
        Sync.cpp)

target_include_directories(
    core
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/../../ # Expose the "engine" root folder
)

target_link_libraries(
    core
    PUBLIC
        Vulkan::Vulkan
)

target_link_libraries(
    core
    PRIVATE
        utils
        fmt::fmt
        Vulkan::Vulkan
        fmtlog
)

target_compile_features(core PUBLIC cxx_std_17)

target_compile_definitions(core PRIVATE
  $<$<CONFIG:Debug>:DEBUG_BUILD>
)

target_compile_options(core PUBLIC /EHsc /Zi)
target_link_options(core PUBLIC /DEBUG:FULL)