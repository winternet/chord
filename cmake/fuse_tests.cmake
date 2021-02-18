find_program(BASH bash)

if (BASH)
  enable_testing()
  set(TEST_TARGET "${PROJECT_NAME}_test_fuse")
  add_test(${TEST_TARGET} ${BASH} ${CMAKE_CURRENT_SOURCE_DIR}/test/fuse/fuse_test.sh)
endif(BASH)

