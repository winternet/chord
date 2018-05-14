find_program(TIDY clang-tidy)

set(TIDY_ARGS
   -format-style=google 
   -checks="*","-fuchsia-*","-hicpp-braces-around-statements","-readability-braces-around-statements","-llvm-include-order","-cppcoreguidelines-owning-memory","-google-readability-braces-around-statements","-readability-avoid-const-params-in-decls","-hicpp-special-member-functions","-cppcoreguidelines-special-member-functions","-llvm-header-guard"
   -header-filter=".*" 
   -p ./build/ ../src/chord.cc)

add_custom_target(tidy COMMAND ${TIDY} ${TIDY_ARGS})
