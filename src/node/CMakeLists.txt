#
# compile executable
# 
add_executable(${PROJECT_NAME} node.cc)
install(
  TARGETS ${PROJECT_NAME} 
  DESTINATION bin
)


#
# add libraries for executable (chord++)
#
target_link_libraries(${PROJECT_NAME} 
  debug asan
  general ${PROJECT_NAME}++)
