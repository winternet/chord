#
# build the fuse adapter for chord
#
set(FUSE_TARGET ${PROJECT_NAME}_fuse)

include(FindPkgConfig)
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

pkg_check_modules(FUSE REQUIRED fuse3)
if(FUSE_FOUND)
  message(STATUS "Found fuse (include: ${FUSE_INCLUDE_DIRS}, library: ${FUSE_LIBRARIES})")
endif()

file(GLOB SOURCES fuse/*.cc)

add_executable(${FUSE_TARGET} ${SOURCES})

target_link_libraries(${FUSE_TARGET}
  debug asan
  general ${PROJECT_NAME}++ ${signals_LIBRARY} ${CONAN_LIBS} ${FUSE_LIBRARIES})
target_include_directories(${FUSE_TARGET} PRIVATE ${FUSE_INCLUDE_DIRS})
set_target_properties(${FUSE_TARGET} PROPERTIES COMPILE_FLAGS "-D_FILE_OFFSET_BITS=64")
