file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(YakiCore STATIC ${SOURCES})
add_library(yaki::core ALIAS YakiCore)

target_include_directories(YakiCore PUBLIC ${CMAKE_SOURCE_DIR}/include/YakiEngine/Core ${CMAKE_SOURCE_DIR}/include/YakiEngine/Ecs)

FetchContent_MakeAvailable(SDL3)
FetchContent_MakeAvailable(stb)
FetchContent_MakeAvailable(hecs)
FetchContent_MakeAvailable(glm)
FetchContent_MakeAvailable(fastgltf)

find_package(imgui CONFIG REQUIRED)

target_link_libraries(YakiCore PUBLIC
  SDL3::SDL3
  imgui::imgui
  glm::glm
  fastgltf::fastgltf
  yaki::render
  hecs::hecs
)
