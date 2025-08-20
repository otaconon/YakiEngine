file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB_RECURSE HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
add_executable(YakiEngine ${SOURCES} ${HEADERS} "Vulkan/Impl/stb_image_impl.cpp")

FetchContent_MakeAvailable(hecs)
FetchContent_MakeAvailable(glm)

find_package(glad REQUIRED)
find_package(Freetype REQUIRED)
find_package(Vulkan REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(fastgltf CONFIG REQUIRED)
find_package(Tracy CONFIG REQUIRED)

target_link_libraries(YakiEngine PRIVATE HECS Vulkan::Headers GPUOpen::VulkanMemoryAllocator Freetype::Freetype SDL3::SDL3 glm::glm Vulkan::Vulkan imgui::imgui fastgltf::fastgltf Tracy::TracyClient)
target_include_directories(YakiEngine PRIVATE ${Stb_INCLUDE_DIR})

add_custom_target(compile_shader
        COMMAND glslc ../shaders/vertex/basic_mesh.vert -o ../shaders/vertex/vert.spv
        COMMENT "Compiling shader: first_triangle.vert"
)