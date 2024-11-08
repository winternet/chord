#
# add sources
#
file(GLOB_RECURSE INTEGRATION_TEST_SOURCES *.cc)
string(REPLACE ";" "\n--   " INTEGRATION_TEST_SOURCES_OUT "${INTEGRATION_TEST_SOURCES}")
message(STATUS "Found integration tests:\n--   ${INTEGRATION_TEST_SOURCES_OUT}")

set(TEST_TARGET "${PROJECT_NAME}_integration_test")

#
# build integration tests
#
add_executable(${TEST_TARGET} ${INTEGRATION_TEST_SOURCES})
target_link_libraries(${TEST_TARGET}
  ${PROJECT_NAME}++
  ${PROJECT_NAME}_test_util
  gtest::gtest
  --coverage
)

#
# add integration tests
#
add_test(NAME ${TEST_TARGET} COMMAND chord_integration_test --gtest_output=xml:gtest-out/chord_integration_test.xml)
