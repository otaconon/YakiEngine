# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "C:/dev/GameEngines/YakiEngine/build/debug/_deps/benchmark-src")
  file(MAKE_DIRECTORY "C:/dev/GameEngines/YakiEngine/build/debug/_deps/benchmark-src")
endif()
file(MAKE_DIRECTORY
  "C:/dev/GameEngines/YakiEngine/build/debug/_deps/benchmark-build"
  "C:/dev/GameEngines/YakiEngine/build/debug/_deps/benchmark-subbuild/benchmark-populate-prefix"
  "C:/dev/GameEngines/YakiEngine/build/debug/_deps/benchmark-subbuild/benchmark-populate-prefix/tmp"
  "C:/dev/GameEngines/YakiEngine/build/debug/_deps/benchmark-subbuild/benchmark-populate-prefix/src/benchmark-populate-stamp"
  "C:/dev/GameEngines/YakiEngine/build/debug/_deps/benchmark-subbuild/benchmark-populate-prefix/src"
  "C:/dev/GameEngines/YakiEngine/build/debug/_deps/benchmark-subbuild/benchmark-populate-prefix/src/benchmark-populate-stamp"
)

set(configSubDirs Debug)
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/dev/GameEngines/YakiEngine/build/debug/_deps/benchmark-subbuild/benchmark-populate-prefix/src/benchmark-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/dev/GameEngines/YakiEngine/build/debug/_deps/benchmark-subbuild/benchmark-populate-prefix/src/benchmark-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
