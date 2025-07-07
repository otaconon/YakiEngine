find_package(Vulkan REQUIRED)
set(GLSLC ${Vulkan_GLSLC_EXECUTABLE})

file(GLOB_RECURSE SHADER_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.frag"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.comp"
)

set(SPIRV_FILES "")

foreach(SHADER_FILE ${SHADER_SOURCES})
    # Replace the source dir with the build dir for SPIR-V outputs
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
