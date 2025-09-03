cmake_minimum_required (VERSION 3.23)

project ("YakiEngine")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/MP)
endif()

include(dependencies.cmake)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

add_subdirectory("Src/Core")
add_subdirectory("Src/Render")
add_subdirectory("Src/App")