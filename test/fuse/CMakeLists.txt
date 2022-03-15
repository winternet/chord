find_program(BASH bash)

if (BASH)
  set(TEST_TARGET "${PROJECT_NAME}_test_fuse")
  add_test(NAME ${TEST_TARGET} 
    COMMAND ${BASH} ${CMAKE_CURRENT_SOURCE_DIR}/fuse_test.sh
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
endif(BASH)

