file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(YakiRender STATIC ${SOURCES} "${CMAKE_CURRENT_SOURCE_DIR}/Vulkan/Impl/stb_image_impl.cpp")
add_library(yaki::render ALIAS YakiRender)

target_include_directories(YakiRender PUBLIC ${CMAKE_SOURCE_DIR}/include/YakiEngine/Render ${Stb_INCLUDE_DIR})

FetchContent_MakeAvailable(VulkanMemoryAllocator)
FetchContent_MakeAvailable(SDL3)
FetchContent_MakeAvailable(stb)

find_package(Vulkan REQUIRED)
find_package(imgui CONFIG REQUIRED)

target_link_libraries(YakiRender PUBLIC
  Vulkan::Vulkan
  Vulkan::Headers
  GPUOpen::VulkanMemoryAllocator
  SDL3::SDL3
  imgui::imgui
)
