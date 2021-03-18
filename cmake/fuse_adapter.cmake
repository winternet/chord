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

if(chord_FUSE_VERSION)
  message(STATUS "Using fuse api version ${chord_FUSE_VERSION}")
  target_compile_definitions(${FUSE_TARGET} PRIVATE FUSE_USE_VERSION=${chord_FUSE_VERSION})
endif()
#set_target_properties(${FUSE_TARGET} PROPERTIES COMPILE_FLAGS "-D_FILE_OFFSET_BITS=64")

if(CMAKE_COMPILER_IS_GNUCC AND ENABLE_COVERAGE AND chord_BUILD_FUSE_ADAPTER)
  target_compile_definitions(${FUSE_TARGET} PRIVATE CHORD_GCOV_FLUSH=1)
endif()

install(TARGETS ${FUSE_TARGET} DESTINATION "/usr/bin/")
