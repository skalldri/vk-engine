find_package(Vulkan REQUIRED COMPONENTS glslc)
find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)

file(GLOB SHADERS
  ${CMAKE_CURRENT_LIST_DIR}/*.vert
  ${CMAKE_CURRENT_LIST_DIR}/*.frag
  ${CMAKE_CURRENT_LIST_DIR}/*.comp
  ${CMAKE_CURRENT_LIST_DIR}/*.geom
  ${CMAKE_CURRENT_LIST_DIR}/*.tesc
  ${CMAKE_CURRENT_LIST_DIR}/*.tese
  ${CMAKE_CURRENT_LIST_DIR}/*.mesh
  ${CMAKE_CURRENT_LIST_DIR}/*.task
  ${CMAKE_CURRENT_LIST_DIR}/*.rgen
  ${CMAKE_CURRENT_LIST_DIR}/*.rchit
  ${CMAKE_CURRENT_LIST_DIR}/*.rmiss)

set(SHADERS_BINARY_DIR ${PROJECT_BINARY_DIR}/shaders)

add_custom_command(
    COMMAND
        ${CMAKE_COMMAND} -E make_directory ${SHADERS_BINARY_DIR}
    OUTPUT ${SHADERS_BINARY_DIR}
    COMMENT "Creating ${SHADERS_BINARY_DIR}"
)

message(STATUS "GLSLC: ${glslc_executable}")

foreach(source IN LISTS SHADERS)
  get_filename_component(FILENAME ${source} NAME)

  message(STATUS "Shader Found: ${FILENAME}")

  add_custom_command(
    COMMAND
      ${glslc_executable}
      -o ${SHADERS_BINARY_DIR}/${FILENAME}.spv
      ${source}
    OUTPUT ${SHADERS_BINARY_DIR}/${FILENAME}.spv
    DEPENDS ${source} ${SHADERS_BINARY_DIR}
    COMMENT "Compiling ${FILENAME}"
  )
  list(APPEND SPV_SHADERS ${SHADERS_BINARY_DIR}/${FILENAME}.spv)
endforeach()

add_custom_target(shaders ALL DEPENDS ${SPV_SHADERS})

message(STATUS "Shaders: ${SHADERS}")
message(STATUS "SPV Shaders: ${SPV_SHADERS}")
message(STATUS "Writing Shaders to ${SHADERS_BINARY_DIR}")