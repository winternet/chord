#
# compile the test utils
#
file(GLOB TEST_UTIL_SOURCES *.cc)
string(REPLACE ";" "\n--   " TEST_UTIL_SOURCES_OUT "${TEST_UTIL_SOURCES}")
message(STATUS "Found test utils:\n--   ${TEST_UTIL_SOURCES_OUT}")

add_library(${PROJECT_NAME}_test_util OBJECT ${TEST_UTIL_SOURCES})
target_link_libraries(${PROJECT_NAME}_test_util ${PROJECT_NAME}++)
