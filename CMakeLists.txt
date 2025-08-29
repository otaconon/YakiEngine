cmake_minimum_required (VERSION 3.23)

project ("YakiEngine")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(dependencies.cmake)

add_subdirectory("Src/Core")
add_subdirectory("Src/Render")
add_subdirectory("Src/App")