#
# build the fuse adapter for chord
#
set(FUSE_TARGET ${PROJECT_NAME}_fuse)

find_package(fusepp REQUIRED)
if(FUSE_FOUND)
  message(STATUS "Found fuse (include: ${FUSE_INCLUDE_DIRS}, library: ${FUSE_LIBRARIES})")
endif()

file(GLOB SOURCES *.cc)
string(REPLACE ";" "\n--   " SOURCES_OUT "${SOURCES}")
message(STATUS "Found fuse source files:\n--   ${SOURCES_OUT}")

add_executable(${FUSE_TARGET} ${SOURCES})
install(
  TARGETS ${FUSE_TARGET} 
  DESTINATION bin
)

target_link_libraries(${FUSE_TARGET}
  debug asan
  general ${PROJECT_NAME}++ fusepp::fusepp
)
set_target_properties(${FUSE_TARGET} PROPERTIES COMPILE_FLAGS "-D_FILE_OFFSET_BITS=64")

if(CMAKE_COMPILER_IS_GNUCC AND ENABLE_COVERAGE AND chord_BUILD_FUSE_ADAPTER)
  target_compile_definitions(${FUSE_TARGET} PRIVATE CHORD_GCOV_FLUSH=1)
endif()
