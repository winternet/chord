cmake_minimum_required(VERSION 3.10)

project(chord
  VERSION "0.0.1" 
  HOMEPAGE_URL "https://github.com/winternet/chord"
  DESCRIPTION "Highly experimental layered distributed peer to peer overlay filesystem based on distributed hashtables (DHTs)."
)

#
# load conan's toolchain file
#
include("${CMAKE_BINARY_DIR}/conan_toolchain.cmake")


configure_file(${CMAKE_SOURCE_DIR}/cmake/project.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/project.h)

option(chord_BUILD_NODE "build the node binary" ON)
option(chord_BUILD_FUSE_ADAPTER "build fuse adapter" ON)
option(chord_BUILD_TESTS "build unit tests" OFF)
option(chord_BUILD_INTEGRATION_TESTS "build integration tests" OFF)
option(chord_USE_CCACHE "use ccache for subsequent builds" ON)


message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")

#
# custom modules / conan
#
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})  # required for conan find_package
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})  # required for conan find_package
string(REPLACE ";" "\n--   " CMAKE_MODULE_PATH_OUT "${CMAKE_MODULE_PATH}")
message(STATUS "Including cmake modules from: \n--   ${CMAKE_MODULE_PATH_OUT}")


#
# add includes
#
include(compile)
include(cppcheck)
include(clang-tidy)
include(package)


#
# generate messages and stubs
#
add_subdirectory(proto)

#
# build library and node
#
add_subdirectory(src)

#
# build fuse adapter
#
if(chord_BUILD_FUSE_ADAPTER)
  add_subdirectory(fuse)
endif()

#
# build tests (unit/integration/fuse)
#
if(chord_BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()

