cmake_minimum_required (VERSION 3.23)

project ("YakiEngine")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(dependencies.cmake)

add_subdirectory("src")
add_subdirectory("shaders")

file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
add_dependencies(YakiEngine Shaders)

