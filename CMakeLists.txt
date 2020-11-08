
cmake_minimum_required(VERSION 3.8)
# Clear VERSION variables when no VERSION is given to project()
if (POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif (POLICY CMP0048)

set(PROJECT "chord")
project(${PROJECT} VERSION "0.0.1")
configure_file(${CMAKE_SOURCE_DIR}/cmake/project.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/project.h)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(GRPC_ALLOW_EXCEPTIONS "allow grpc exceptions" OFF)

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

#-Wshadow
#-Wuseless-cast
add_compile_options(
  -fPIC
  -Wall
  -Wextra
  -Wnon-virtual-dtor
  -Wold-style-cast
  -Wunused
  -Woverloaded-virtual
  #-Wpedantic
  -Wconversion
  -Wmisleading-indentation
  -Wduplicated-cond
  -Wduplicated-branches
  -Wlogical-op
  -Wnull-dereference
  -Wdouble-promotion
  -Wformat=2
)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
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
include(CMakeLists-Externals.txt)


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

#
# find packages
#
find_package(Boost REQUIRED COMPONENTS thread system program_options serialization)
find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(RocksDB REQUIRED)
find_package(YamlCpp REQUIRED)
find_package(FsWatch REQUIRED)


#
# generate messages and services
#
file(GLOB ProtoFiles "${ProtoPath}/*.proto")
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


#
# create shared library chord++
#
add_library(${PROJECT_NAME}++ SHARED ${SOURCES} ${SOURCES_GEN} ${ProtoSources} ${ProtoHeaders} ${GrpcSources} ${GrpcHeaders})
target_link_libraries(${PROJECT_NAME}++ ${Boost_LIBRARIES} ${PROTOBUF_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} ${FsWatch_LIBRARY} ${RocksDB_LIBRARY} ${YAMLCPP_LIBRARY}
  #debug asan
  general stdc++fs grpc++ grpc ssl crypto)
target_link_libraries(${PROJECT_NAME}++ --coverage)
install(TARGETS ${PROJECT_NAME}++ LIBRARY DESTINATION lib)


#
# compile executable
# 
add_executable(${PROJECT_NAME} src/chord.cc)

#
# libsignals library (CMakeLists-Externals)
#
ExternalProject_Get_Property(signals BINARY_DIR)
set(signals_LIBRARY "${BINARY_DIR}/libsignals.a")
set(signals_INCLUDE_DIR "${EXTERNAL_PROJECTS}/signals/include")
include_directories(${signals_INCLUDE_DIR})



#
# add libraries for executable (chord++)
#
target_link_libraries(${PROJECT_NAME} 
  debug asan
  general ${PROJECT_NAME}++ ${signals_LIBRARY})

#
# add external project dependencies
#
add_dependencies(${PROJECT_NAME}++ spdlog)
add_dependencies(${PROJECT_NAME}++ signals)

#
# fuse adapter
#
if(chord_BUILD_FUSE_ADAPTER)
  include(fuse_adapter)
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
