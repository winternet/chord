find_program(CPPCHECK cppcheck)

set(CPPCHECK_ARGS 
          --enable=all
          --std=c++11
          --language=c++
          --inline-suppr
          --suppress=noExplicitConstructor
          --suppress=passedByValue
          --suppress=*:*gen/chord*
          --template="[{severity}];;[{id}];;{message};;{callstack};;\({file}:{line}\)"
          --verbose
          --quiet)

if(CMAKE_EXPORT_COMPILE_COMMANDS)
  add_custom_target(cppcheck COMMAND ${CPPCHECK} ${CPPCHECK_ARGS} --project=compile_commands.json)
else()
  file(GLOB_RECURSE ALL_SOURCE_FILES *.cpp *.cc *.h *.hpp)
  add_custom_target(cppcheck COMMAND ${CPPCHECK} ${CPPCHECK_ARGS} ${ALL_SOURCE_FILES})
endif()
