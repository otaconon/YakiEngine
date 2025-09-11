# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "C:/dev/GameEngines/YakiEngine/build/debug/_deps/hecs-src")
  file(MAKE_DIRECTORY "C:/dev/GameEngines/YakiEngine/build/debug/_deps/hecs-src")
endif()
file(MAKE_DIRECTORY
  "C:/dev/GameEngines/YakiEngine/build/debug/_deps/hecs-build"
  "C:/dev/GameEngines/YakiEngine/build/debug/_deps/hecs-subbuild/hecs-populate-prefix"
  "C:/dev/GameEngines/YakiEngine/build/debug/_deps/hecs-subbuild/hecs-populate-prefix/tmp"
  "C:/dev/GameEngines/YakiEngine/build/debug/_deps/hecs-subbuild/hecs-populate-prefix/src/hecs-populate-stamp"
  "C:/dev/GameEngines/YakiEngine/build/debug/_deps/hecs-subbuild/hecs-populate-prefix/src"
  "C:/dev/GameEngines/YakiEngine/build/debug/_deps/hecs-subbuild/hecs-populate-prefix/src/hecs-populate-stamp"
)

set(configSubDirs Debug)
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/dev/GameEngines/YakiEngine/build/debug/_deps/hecs-subbuild/hecs-populate-prefix/src/hecs-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/dev/GameEngines/YakiEngine/build/debug/_deps/hecs-subbuild/hecs-populate-prefix/src/hecs-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
