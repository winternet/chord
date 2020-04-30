# - Find fswatch
#
# FsWatch_FOUND, if false, do not try to link to fswatch
# FsWatch_LIBRARY, where to find fswatch
# FsWatch_INCLUDE, where to find fswatch headers
#
# Look for the header file.
find_path(FsWatch_INCLUDE libfswatch/c++/monitor_factory.hpp
    /usr/local/include/
    /usr/include/
    DOC "Path in which the file monitor_factory.h is located.")

# find the FsWatch library
find_library(FsWatch_LIBRARY
    NAMES fswatch)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FsWatch DEFAULT_MSG FsWatch_INCLUDE FsWatch_LIBRARY)

if(FsWatch_FOUND)
  message(STATUS "Found fswatch (include: ${FsWatch_INCLUDE}, library: ${FsWatch_LIBRARY})")
  set(FsWatch_INCLUDE ${FsWatch_INCLUDE})
  set(FsWatch_LIBRARY ${FsWatch_LIBRARY})
  mark_as_advanced(FsWatch_INCLUDE FsWatch_LIBRARY)
endif()
