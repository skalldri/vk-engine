
add_library(core-win32 STATIC)

target_sources(
  core-win32
  PRIVATE
    GlfwWindowSystem.cpp
)

target_include_directories(
  core-win32
  PUBLIC
    ../../ # src folder 
)

target_link_libraries(
  core-win32
  PUBLIC
    Vulkan::Vulkan
    fmt::fmt
    fmtlog
    glfw
)

target_compile_features(core-win32 PUBLIC cxx_std_17)

target_compile_definitions(core-win32 PRIVATE
  $<$<CONFIG:Debug>:DEBUG_BUILD>
)

target_compile_options(core-win32 PUBLIC /EHsc /Zi)
target_link_options(core-win32 PUBLIC /DEBUG:FULL)