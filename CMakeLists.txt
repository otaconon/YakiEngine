cmake_minimum_required (VERSION 3.23)

project ("YakiEngine")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(dependencies.cmake)

add_subdirectory("src/Core")
add_subdirectory("src/Render")
add_subdirectory("src/App")