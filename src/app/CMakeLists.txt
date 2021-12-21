add_subdirectory(shaders)

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
        core-win32
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

# First for the generic no-config case (e.g. with mingw)
set_target_properties( engine PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR} )
set_target_properties( engine PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR} )
set_target_properties( engine PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR} )
set_target_properties( engine PROPERTIES PDB_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR} )
set_target_properties( engine PROPERTIES COMPILE_PDB_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR} )

# Second, for multi-config builds (e.g. msvc)
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set_target_properties( engine PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PROJECT_BINARY_DIR} )
    set_target_properties( engine PROPERTIES LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PROJECT_BINARY_DIR} )
    set_target_properties( engine PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PROJECT_BINARY_DIR} )
    set_target_properties( engine PROPERTIES PDB_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PROJECT_BINARY_DIR} )
    set_target_properties( engine PROPERTIES COMPILE_PDB_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PROJECT_BINARY_DIR} )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )