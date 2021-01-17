#
# build the fuse adapter for chord
#
include(FindPkgConfig)
set(CMAKE_CXX_FLAGS "${CMAKE_INCLUDE_DIRS} -D_FILE_OFFSET_BITS=64")

pkg_check_modules(FUSE REQUIRED fuse3)
if(FUSE_FOUND)
  message(STATUS "Found fuse (include: ${FUSE_INCLUDE_DIRS}, library: ${FUSE_LIBRARIES})")
endif()

file(GLOB SOURCES fuse/*.cc)
add_executable("${PROJECT_NAME}_fuse" ${SOURCES})

target_link_libraries(${PROJECT_NAME}_fuse
  debug asan
  general ${PROJECT_NAME}++ ${signals_LIBRARY} ${FUSE_LIBRARIES})

include_directories(${fusepp_INCLUDE_DIR})
include_directories(${FUSE_INCLUDE_DIRS})
add_dependencies(${PROJECT_NAME}_fuse fusepp)
