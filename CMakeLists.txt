project(chord)
cmake_minimum_required(VERSION 2.8)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

set(ProtoPath "${CMAKE_CURRENT_SOURCE_DIR}/proto")
set(ProtoGenerateTargetPath "./gen")

set(CMAKE_BUILD_TYPE 				Debug)

set(CMAKE_CXX_COMPILER      "/usr/bin/clang++")

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  add_compile_options(-std=c++17)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  add_compile_options(-std=c++1z)
endif()

add_compile_options(
  -Wall
  -Wextra
)
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  # compile with address sanitizer (2x slowdown)
  add_compile_options(
    -fsanitize=address -fsanitize-address-use-after-scope
  )
#add_compile_options(
#    $<$<NOT:$<STREQUAL:$<TARGET_PROPERTY:SOURCE_DIR>,${CMAKE_SOURCE_DIR}/test_app1>>:-Werror>
#  )
endif()


#
# custom modules
#
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
message(STATUS "Including custom cmake modules from ${CMAKE_MODULE_PATH}")

include(grpc)
include(protobuf)
include(CTest)

#
# find packages
#
find_package(Boost COMPONENTS log thread system program_options)
find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

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
  general stdc++fs grpc++ grpc ssl crypto)
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


### TESTS ###
###
### trivial example using gtest and gmock
###
file(GLOB TEST_SOURCES test/*.cc)
string(REPLACE ";" "\n--   " TEST_SOURCES_OUT "${TEST_SOURCES}")
message(STATUS "Found tests:\n--   ${TEST_SOURCES_OUT}")
#
enable_testing()
include_directories(include)
add_executable(chord_test ${TEST_SOURCES})
target_link_libraries(chord_test 
  debug asan
  general gtest gmock gmock_main ${PROJECT_NAME}++)
add_test(NAME chord_test COMMAND chord_test)
