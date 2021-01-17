file(GLOB TEST_SOURCES test/*.cc)
file(GLOB TEST_UTIL_SOURCES test/util/*.cc)
set(TEST_SOURCES ${TEST_SOURCES} ${TEST_UTIL_SOURCES})

conan_basic_setup()

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
  general ${CONAN_LIBS} ${PROJECT_NAME}++ ${signals_LIBRARY})
target_link_libraries(${TEST_TARGET} --coverage)
add_test(NAME ${TEST_TARGET} COMMAND chord_test --gtest_output=xml:gtest-out/chord_test.xml)
