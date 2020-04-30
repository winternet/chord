# - Find yaml-cpp
#
# YAMLCPP_FOUND, if false, do not try to link to yaml-cpp
# YAMLCPP_LIBRARY, where to find yaml-cpp
# YAMLCPP_INCLUDE_DIR, where to find yaml.h
#
# Look for the header file.
find_path(YAMLCPP_INCLUDE_DIR yaml-cpp/yaml.h
    PATHS
    /opt/local/include
    /usr/local/include/yaml-cpp/
    /usr/local/include/
    /usr/include/yaml-cpp/
    /usr/include/
    /sw/yaml-cpp/ # Fink
    /opt/local/yaml-cpp/ # DarwinPorts
    /opt/csw/yaml-cpp/ # Blastwave
    /opt/yaml-cpp/
    DOC "Path in which the file yaml-cpp/yaml.h is located.")

## attempt to find static library first if this is set
if(YAMLCPP_STATIC_LIBRARY)
    set(YAMLCPP_STATIC libyaml-cpp.a)
endif()

# find the yaml-cpp library
find_library(YAMLCPP_LIBRARY
    NAMES ${YAMLCPP_STATIC} yaml-cpp
    PATH_SUFFIXES lib64 lib
    PATHS ~/Library/Frameworks
    /Library/Frameworks
    /usr/local
    /usr
    /sw
    /opt/local
    /opt/csw
    /opt
    ${YAMLCPP_DIR}/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(YamlCpp DEFAULT_MSG YAMLCPP_INCLUDE_DIR YAMLCPP_LIBRARY)

if(YAMLCPP_FOUND)
  message(STATUS "Found YamlCpp (include: ${YAMLCPP_INCLUDE_DIR}, library: ${YAMLCPP_LIBRARY})")
  mark_as_advanced(LevelDB_INCLUDE_DIR LevelDB_LIBRARY)
endif()
