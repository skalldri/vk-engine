add_executable(engine)

target_sources(
    engine
    PRIVATE
        main.cpp
)

target_link_libraries(
    engine
    PRIVATE
        glm
        glfw
        fmt::fmt
        fmtlog
        CLI11::CLI11
        Vulkan::Vulkan
        core
        utils
)

add_dependencies(engine shaders)

target_compile_features(engine PUBLIC cxx_std_17)

# Works correctly for both single and multi-config generators
target_compile_definitions(engine PRIVATE
  $<$<CONFIG:Debug>:DEBUG_BUILD>
)

target_compile_options(engine PUBLIC /EHsc /Zi)
target_link_options(engine PUBLIC /DEBUG:FULL)