find_package(GTest REQUIRED)

#
# redirect test artifacts to test_bin folder
#
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test_bin)

add_subdirectory(util)


if(chord_BUILD_FUSE_ADAPTER)
  add_subdirectory(fuse)
endif()

if(chord_BUILD_INTEGRATION_TESTS)
  add_subdirectory(integration)
endif()


#
# add sources
#
file(GLOB TEST_SOURCES *.cc)
string(REPLACE ";" "\n--   " TEST_SOURCES_OUT "${TEST_SOURCES}")
message(STATUS "Found tests:\n--   ${TEST_SOURCES_OUT}")

#
# build unit tests
#
set(TEST_TARGET "${PROJECT_NAME}_test")
add_executable(${TEST_TARGET} ${TEST_SOURCES})
target_link_libraries(${TEST_TARGET}
  debug asan
  ${PROJECT_NAME}++
  ${PROJECT_NAME}_test_util
  GTest::GTest
  # TODO remove --coverage(enabled globally)
  --coverage
)

#
# add unit tests
#
add_test(NAME ${TEST_TARGET} COMMAND chord_test --gtest_output=xml:gtest-out/chord_test.xml)
