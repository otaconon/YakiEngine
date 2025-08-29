file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(YakiEngine ${SOURCES})
include_directories(${CMAKE_SOURCE_DIR}/include/YakiEngine/App)

FetchContent_MakeAvailable(VulkanMemoryAllocator)
FetchContent_MakeAvailable(SDL3)
FetchContent_MakeAvailable(stb)
FetchContent_MakeAvailable(hecs)
FetchContent_MakeAvailable(glm)

find_package(imgui)

target_link_libraries(YakiEngine PRIVATE
  imgui::imgui
  glm::glm
  hecs::hecs
  yaki::render
  yaki::core
)

set(GLSLC ${Vulkan_GLSLC_EXECUTABLE})

file(GLOB_RECURSE SHADER_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/*.vert"
  "${CMAKE_CURRENT_SOURCE_DIR}/*.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/*.comp"
)

set(SPIRV_FILES "")

foreach(SHADER_FILE ${SHADER_SOURCES})
  file(RELATIVE_PATH REL_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${SHADER_FILE})
  set(SPIRV_FILE "${CMAKE_CURRENT_BINARY_DIR}/${REL_PATH}.spv")
  list(APPEND SPIRV_FILES ${SPIRV_FILE})

  add_custom_command(
    OUTPUT  ${SPIRV_FILE}
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:YakiEngine>/shaders"
    COMMAND ${GLSLC} ${SHADER_FILE} -o ${SPIRV_FILE}
    DEPENDS ${SHADER_FILE}
    COMMENT "Compiling shader: ${REL_PATH}"
    VERBATIM
  )
endforeach()

add_custom_target(Shaders ALL DEPENDS ${SPIRV_FILES})

add_dependencies(YakiEngine Shaders)
