project(chord)
cmake_minimum_required(VERSION 2.8)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

set(ProtoPath "${CMAKE_CURRENT_SOURCE_DIR}/proto")
set(ProtoGenerateTargetPath "gen")

set(CMAKE_BUILD_TYPE 				Debug)
#set(CMAKE_CXX_COMPILER      "/usr/bin/clang++")
set(CMAKE_CXX_FLAGS 				"${CMAKE_CXX_FLAGS} -Wall")

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

if( Boost_FOUND )
  add_definitions(-std=c++14)
endif()


#
# generate messages and services
#
file(GLOB ProtoFiles "${ProtoPath}/*.proto")
protobuf_generate_cpp_with_path(${ProtoGenerateTargetPath} ProtoSources ProtoHeaders ${ProtoFiles})
message(STATUS "Generating protobuf messages")
protobuf_generate_grpc_cpp_with_path(${ProtoGenerateTargetPath} GrpcSources GrpcHeaders ${ProtoFiles})
message(STATUS "Generating grpc services")
include_directories(${ProtoGenerateTargetPath})
message(STATUS "Including generated header files")

file(GLOB SOURCES_GEN ${ProtoGenerateTargetPath}/*.cc)
string(REPLACE ";" "\n--   " SOURCES_GEN_OUT "${SOURCES_GEN}")
message(STATUS "Found generated source files:\n--   ${SOURCES_GEN_OUT}")


#
# add source files
#
file(GLOB SOURCES src/*.cc)
include_directories(include)
include_directories("${CMAKE_CURRENT_BINARY_DIR}/gen")

#
# create shared library chord++
#
add_library(${PROJECT_NAME}++ SHARED ${SOURCES} ${SOURCES_GEN} ${ProtoSources} ${ProtoHeaders} ${GrpcSources} ${GrpcHeaders})
target_link_libraries(${PROJECT_NAME}++ ${Boost_LIBRARIES} ${PROTOBUF_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} grpc++ grpc ssl crypto)
install(TARGETS ${PROJECT_NAME}++ LIBRARY DESTINATION lib)

#
# compile executable
# 
add_executable(${PROJECT_NAME} src/chord.cc)


#
# add libraries for executable (chord++)
#
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}++)


### TESTS ###
###
### trivial example using gtest and gmock
###
file(GLOB TEST_SOURCES test/*.cc)
string(REPLACE ";" "\n--   " TEST_SOURCES_OUT "${TEST_SOURCES}")
message(STATUS "Found tests:\n--   ${TEST_SOURCES_OUT}")
#
include_directories(include)
add_executable(test_chord ${TEST_SOURCES})
target_link_libraries(test_chord gtest gmock gmock_main ${PROJECT_NAME}++)
add_test(NAME test COMMAND test_chord)
