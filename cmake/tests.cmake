file(GLOB_RECURSE TEST_SOURCES test/*.cc)
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
  general gtest gmock gmock_main ${PROJECT_NAME}++ ${signals_LIBRARY})
target_link_libraries(${TEST_TARGET} --coverage)
add_test(NAME ${TEST_TARGET} COMMAND chord_test --gtest_output=xml:gtest-out/chord_test.xml)
