# - Find fswatch
#
# FsWatch_FOUND, if false, do not try to link to fswatch
# FsWatch_LIBRARY, where to find fswatch
# FsWatch_INCLUDE_DIR, where to find fswatch headers
#

# Look for the header file.
find_path(
    FsWatch_INCLUDE_DIR 
    NAMES libfswatch/c++/monitor.hpp
    DOC "Path in which the file monitor_factory.h is located.")

# find the FsWatch library
find_library(FsWatch_LIBRARY
    NAMES fswatch
    PATH_SUFFIXES libfswatch)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FsWatch DEFAULT_MSG FsWatch_INCLUDE_DIR FsWatch_LIBRARY)

if(FsWatch_FOUND)
  message(STATUS "Found fswatch (include: ${FsWatch_INCLUDE_DIR}, library: ${FsWatch_LIBRARY})")
  mark_as_advanced(FsWatch_INCLUDE_DIR FsWatch_LIBRARY)
endif()
