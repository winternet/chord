find_program(CPPCHECK cppcheck)

file(GLOB_RECURSE ALL_SOURCE_FILES *.cpp *.cc *.h *.hpp)

#cppcheck --enable=all --std=c++11 -I ./include -i ./build --verbose --quiet --template="[{severity}][{id}] {message} {callstack} ({file}:{line})"
add_custom_target(
        cppcheck
        COMMAND ${CPPCHECK}
        --enable=all
        --std=c++11
        --language=c++
        --template="[{severity}];;[{id}];;{message};;{callstack};;\({file}:{line}\)"
        --verbose
        --quiet
        ${ALL_SOURCE_FILES}
)
