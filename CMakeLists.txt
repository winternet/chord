#set(CMAKE_CXX_COMPILER      "/usr/bin/clang++")

cmake_minimum_required(VERSION 2.8)
cmake_policy(SET CMP0048 NEW)

set(PROJECT "chord")
project(${PROJECT} VERSION "0.0.1")
configure_file(${CMAKE_SOURCE_DIR}/cmake/project.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/project.h)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(ProtoPath "${CMAKE_CURRENT_SOURCE_DIR}/proto")
set(ProtoGenerateTargetPath "./gen")

set(CMAKE_BUILD_TYPE 				Debug)

if(CMAKE_COMPILER_IS_GNUCC)
  option(ENABLE_COVERAGE "enable coverage reporting for gcc/clang" FALSE)
  if(ENABLE_COVERAGE)
    add_compile_options(--coverage -O0)
  endif()
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
  -Wpedantic
  -Wconversion
  -Wmisleading-indentation
  -Wduplicated-cond
  -Wduplicated-branches
  -Wlogical-op
  -Wnull-dereference
  -Wdouble-promotion
  -Wformat=2
  -std=c++14
)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wlifetime)
endif()
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
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
find_package(LevelDB REQUIRED)
find_package(YamlCpp REQUIRED)

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
target_link_libraries(${PROJECT_NAME}++ ${Boost_LIBRARIES} ${PROTOBUF_LIBRARY} ${CMAKE_THREAD_LIBS_INIT}
  #debug asan
  general stdc++fs grpc++ grpc ssl crypto leveldb yaml-cpp)
target_link_libraries(${PROJECT_NAME}++ --coverage)
install(TARGETS ${PROJECT_NAME}++ LIBRARY DESTINATION lib)


#
# compile executable
# 
add_executable(${PROJECT_NAME} src/chord.cc)


#
# add libraries for executable (chord++)
#
target_link_libraries(${PROJECT_NAME} 
  debug asan
  general ${PROJECT_NAME}++)

#
# add external project dependencies
#
add_dependencies(${PROJECT_NAME} spdlog)
add_dependencies(${PROJECT_NAME}++ spdlog)

#
# tests
#
file(GLOB TEST_SOURCES test/*.cc)
string(REPLACE ";" "\n--   " TEST_SOURCES_OUT "${TEST_SOURCES}")
message(STATUS "Found tests:\n--   ${TEST_SOURCES_OUT}")
#
enable_testing()
set(TEST_TARGET "${PROJECT_NAME}_test")

include_directories(include)
include_directories(SYSTEM "${CMAKE_CURRENT_BINARY_DIR}/gen")
add_executable(${TEST_TARGET} ${TEST_SOURCES})
target_link_libraries(${TEST_TARGET} 
  debug asan
  general gtest gmock gmock_main ${PROJECT_NAME}++)
target_link_libraries(${TEST_TARGET} --coverage)
add_test(NAME ${TEST_TARGET} COMMAND chord_test)
