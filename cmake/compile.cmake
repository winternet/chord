
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

#
# enable coverage
#
if(CMAKE_COMPILER_IS_GNUCC)
  option(ENABLE_COVERAGE "enable coverage reporting for gcc/clang" FALSE)
  if(ENABLE_COVERAGE)
    add_compile_options(--coverage -O0)
  endif()
endif()

if (CMAKE_BUILD_TYPE_LOWER STREQUAL "Debug")
  # compile with address sanitizer (2x slowdown)
  add_compile_options(-fsanitize=address)
endif()

add_compile_options(
  -Wall
  -Wextra
  -Wnon-virtual-dtor
  -Wold-style-cast
  -Wunused
  -Woverloaded-virtual
  -Wconversion
  -Wmisleading-indentation
  -Wduplicated-cond
  -Wduplicated-branches
  -Wlogical-op
  -Wnull-dereference
  -Wdouble-promotion
  -Wformat=2
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wlifetime)
  add_compile_options(-Weverything)
endif()

#
# enable ccache
#
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND AND chord_USE_CCACHE)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif()
