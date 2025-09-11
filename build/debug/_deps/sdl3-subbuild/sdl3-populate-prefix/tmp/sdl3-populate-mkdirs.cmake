# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "C:/dev/GameEngines/YakiEngine/build/debug/_deps/sdl3-src")
  file(MAKE_DIRECTORY "C:/dev/GameEngines/YakiEngine/build/debug/_deps/sdl3-src")
endif()
file(MAKE_DIRECTORY
  "C:/dev/GameEngines/YakiEngine/build/debug/_deps/sdl3-build"
  "C:/dev/GameEngines/YakiEngine/build/debug/_deps/sdl3-subbuild/sdl3-populate-prefix"
  "C:/dev/GameEngines/YakiEngine/build/debug/_deps/sdl3-subbuild/sdl3-populate-prefix/tmp"
  "C:/dev/GameEngines/YakiEngine/build/debug/_deps/sdl3-subbuild/sdl3-populate-prefix/src/sdl3-populate-stamp"
  "C:/dev/GameEngines/YakiEngine/build/debug/_deps/sdl3-subbuild/sdl3-populate-prefix/src"
  "C:/dev/GameEngines/YakiEngine/build/debug/_deps/sdl3-subbuild/sdl3-populate-prefix/src/sdl3-populate-stamp"
)

set(configSubDirs Debug)
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/dev/GameEngines/YakiEngine/build/debug/_deps/sdl3-subbuild/sdl3-populate-prefix/src/sdl3-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/dev/GameEngines/YakiEngine/build/debug/_deps/sdl3-subbuild/sdl3-populate-prefix/src/sdl3-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
