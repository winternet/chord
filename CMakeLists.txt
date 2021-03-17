cmake_minimum_required(VERSION 3.10)

set(PROJECT "chord")
project(${PROJECT} VERSION "0.0.1")

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

configure_file(${CMAKE_SOURCE_DIR}/cmake/project.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/project.h)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_PROJECT_HOMEPAGE_URL "https://github.com/winternet/chord")

set(ProtoPath "${CMAKE_CURRENT_SOURCE_DIR}/proto")
set(ProtoGenerateTargetPath "./gen")

option(chord_BUILD_FUSE_ADAPTER "build fuse adapter" ON)
option(chord_BUILD_TESTS "build unit tests" OFF)
option(chord_BUILD_INTEGRATION_TESTS "build integration tests" OFF)
option(chord_USE_CCACHE "use ccache for subsequent builds" ON)

if(CMAKE_COMPILER_IS_GNUCC)
  option(ENABLE_COVERAGE "enable coverage reporting for gcc/clang" FALSE)
  if(ENABLE_COVERAGE)
    add_compile_options(--coverage -O0)
  endif()
endif()

#
# enable ccache
#
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND AND chord_USE_CCACHE)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif()

add_compile_options(
  -Wall
  -Wextra
  -Wnon-virtual-dtor
  -Wold-style-cast
  -Wunused
  -Woverloaded-virtual
  -Wconversion
  -Wmisleading-indentation
  -Wduplicated-cond
  -Wduplicated-branches
  -Wlogical-op
  -Wnull-dereference
  -Wdouble-promotion
  -Wformat=2
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wlifetime)
  add_compile_options(-Weverything)
endif()
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")

if (CMAKE_BUILD_TYPE_LOWER STREQUAL "Debug")
  # compile with address sanitizer (2x slowdown)
  add_compile_options(-fsanitize=address)
endif()

#
# external dependencies
#
#include(CMakeLists-Externals.txt)


#
# custom modules
#
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
message(STATUS "Including custom cmake modules from ${CMAKE_MODULE_PATH}")

include(grpc)
include(protobuf)
include(CTest)
include(cppcheck)
include(clang-tidy)

set(PROTOBUF_GENERATE_CPP_APPEND_PATH ON)
set(PROTOBUF_PROTOC_EXECUTABLE ${CONAN_BIN_DIRS_PROTOBUF}/protoc)
set(GRPC_CPP_PLUGIN ${CONAN_BIN_DIRS_GRPC}/grpc_cpp_plugin)

#
# generate messages and services
#
file(GLOB ProtoFiles "${ProtoPath}/*.proto")
string(REPLACE ";" "\n--   " PROTO_FILES_OUT "${ProtoFiles}")
message(STATUS "Found proto files: \n--   ${PROTO_FILES_OUT}")
protobuf_generate_cpp_with_path(${ProtoGenerateTargetPath} ProtoSources ProtoHeaders ${ProtoFiles})

message(STATUS "Generating protobuf messages")
protobuf_generate_grpc_cpp_with_path(${ProtoGenerateTargetPath} GrpcSources GrpcHeaders ${ProtoFiles})
message(STATUS "Generating grpc services")
include_directories(SYSTEM ${ProtoGenerateTargetPath})
message(STATUS "Including generated header files: ${ProtoGenerateTargetPath}")

file(GLOB SOURCES_GEN ${CMAKE_CURRENT_BINARY_DIR}/gen/*)
string(REPLACE ";" "\n--   " SOURCES_GEN_OUT "${SOURCES_GEN}")
message(STATUS "Found generated source files:\n--   ${SOURCES_GEN_OUT}")


#
# add source files
#
file(GLOB SOURCES src/*.cc)
include_directories(include)
# use SYSTEM symbol to suppress warnings within generated code
include_directories(SYSTEM "${CMAKE_CURRENT_BINARY_DIR}/gen")

include_directories(${CONAN_INCLUDE_DIRS})


#
# create shared library chord++
#
add_library(${PROJECT_NAME}++ STATIC ${SOURCES} ${SOURCES_GEN} ${ProtoSources} ${ProtoHeaders} ${GrpcSources} ${GrpcHeaders})
#conan_target_link_libraries(${PROJECT_NAME}++ ${CONAN_LIBS} GENERAL stdc++fs)
target_link_libraries(${PROJECT_NAME}++ ${CONAN_LIBS} stdc++fs --coverage)
#target_link_libraries(${PROJECT_NAME}++)
#install(TARGETS ${PROJECT_NAME}++ LIBRARY DESTINATION lib)


#
# compile executable
# 
add_executable(${PROJECT_NAME} src/chord.cc)

##
## libsignals library (CMakeLists-Externals)
##
#ExternalProject_Get_Property(signals BINARY_DIR)
#set(signals_LIBRARY "${BINARY_DIR}/libsignals.a")
#set(signals_INCLUDE_DIR "${EXTERNAL_PROJECTS}/signals/include")
#include_directories(${signals_INCLUDE_DIR})


#
# add libraries for executable (chord++)
#
target_link_libraries(${PROJECT_NAME} 
  debug asan
  general ${PROJECT_NAME}++) # ${signals_LIBRARY})


#
# fuse adapter
#
if(chord_BUILD_FUSE_ADAPTER)
  include(fuse_adapter)
  include(fuse_tests)
endif()

#
# tests
#
if(chord_BUILD_TESTS)
  include(tests)
endif()

#
# integration tests
#
if(chord_BUILD_INTEGRATION_TESTS)
  include(integration_tests)
endif()


#
# cpack
#
install(TARGETS ${PROJECT_NAME} DESTINATION "/usr/bin/")
#install(FILES "config.yml" DESTINATION "/etc/chord/")

set(CPACK_GENERATOR "DEB;RPM")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Experimental layered distributed peer to peer overlay filesystem")
#set(CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_SOURCE_DIR}/README.md")
#set(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_VENDOR "winternet")

# DEB
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "winternet") # required
set(CPACK_DEBIAN_PACKAGE_DEPENDS "fuse3, libfuse3-3")

# RPM
set(CPACK_RPM_PACKAGE_REQUIRES "fuse3")

include(InstallRequiredSystemLibraries)
include(CPack)

