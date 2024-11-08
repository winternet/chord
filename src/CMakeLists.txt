#
# add sources
#
file(GLOB SOURCES *.cc)
string(REPLACE ";" "\n--   " SOURCES_OUT "${SOURCES}")
message(STATUS "Found sources:\n--   ${SOURCES_OUT}")

if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()


#
# add dependencies
#
find_package(spdlog REQUIRED)
find_package(PalSigslot REQUIRED)
find_package(fswatch REQUIRED)
find_package(RocksDB REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(Boost COMPONENTS serialization program_options REQUIRED)


#
# build library
#
add_library(${PROJECT_NAME}++ STATIC ${SOURCES})
target_include_directories(${PROJECT_NAME}++ PUBLIC ../include)
target_link_libraries(${PROJECT_NAME}++
  ${PROJECT_NAME}_grpc
  yaml-cpp::yaml-cpp
  spdlog::spdlog
  fswatch::fswatch
  Boost::serialization
  Boost::program_options
  Pal::Sigslot
  RocksDB::rocksdb
  stdc++fs 
  # TODO remove
  --coverage)

if(chord_BUILD_NODE)
  add_subdirectory(node)
endif()

