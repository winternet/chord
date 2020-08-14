file(GLOB_RECURSE INTEGRATION_TEST_SOURCES test/integration/*.cc)
file(GLOB TEST_UTIL_SOURCES test/util/*.cc)
set(INTEGRATION_TEST_SOURCES ${INTEGRATION_TEST_SOURCES} ${TEST_UTIL_SOURCES})

string(REPLACE ";" "\n--   " INTEGRATION_TEST_SOURCES_OUT "${INTEGRATION_TEST_SOURCES}")
message(STATUS "Found integration tests:\n--   ${INTEGRATION_TEST_SOURCES_OUT}")
#
enable_testing()
set(TEST_TARGET "${PROJECT_NAME}_integration_test")

include_directories(include)
include_directories(SYSTEM "${CMAKE_CURRENT_BINARY_DIR}/gen")
add_executable(${TEST_TARGET} ${INTEGRATION_TEST_SOURCES})
target_link_libraries(${TEST_TARGET} 
  debug asan
  general gtest gmock gmock_main ${PROJECT_NAME}++ ${signals_LIBRARY})
target_link_libraries(${TEST_TARGET} --coverage)
add_test(NAME ${TEST_TARGET} COMMAND chord_integration_test --gtest_output=xml:gtest-out/chord_integration_test.xml)
