# - Find RocksDB
#
#  RocksDB_INCLUDES  - List of RocksDB includes
#  RocksDB_LIBRARIES - List of libraries when using RocksDB.
#  RocksDB_FOUND     - True if RocksDB found.

# Look for the header file.
find_path(RocksDB_INCLUDE NAMES rocksdb/db.h
                          PATHS $ENV{ROCKSDB_ROOT}/include /opt/local/include /usr/local/include /usr/include
                          DOC "Path in which the file rocksdb/db.h is located." )

# Look for the library.
find_library(RocksDB_LIBRARY NAMES rocksdb
                             PATHS /usr/lib $ENV{ROCKSDB_ROOT}/lib
                             DOC "Path to rocksdb library." )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RocksDB DEFAULT_MSG RocksDB_INCLUDE RocksDB_LIBRARY)

if(ROCKSDB_FOUND)
  message(STATUS "Found RocksDB (include: ${RocksDB_INCLUDE}, library: ${RocksDB_LIBRARY})")
  set(RocksDB_INCLUDES ${RocksDB_INCLUDE})
  set(RocksDB_LIBRARIES ${RocksDB_LIBRARY})
  mark_as_advanced(RocksDB_INCLUDE RocksDB_LIBRARY)

  if(EXISTS "${RocksDB_INCLUDE}/rocksdb/db.h")
    file(STRINGS "${RocksDB_INCLUDE}/rocksdb/db.h" __version_lines
           REGEX "static const int k[^V]+Version[ \t]+=[ \t]+[0-9]+;")

    foreach(__line ${__version_lines})
      if(__line MATCHES "[^k]+kMajorVersion[ \t]+=[ \t]+([0-9]+);")
        set(ROCKSDB_VERSION_MAJOR ${CMAKE_MATCH_1})
      elseif(__line MATCHES "[^k]+kMinorVersion[ \t]+=[ \t]+([0-9]+);")
        set(ROCKSDB_VERSION_MINOR ${CMAKE_MATCH_1})
      endif()
    endforeach()

    if(ROCKSDB_VERSION_MAJOR AND ROCKSDB_VERSION_MINOR)
      set(ROCKSDB_VERSION "${ROCKSDB_VERSION_MAJOR}.${ROCKSDB_VERSION_MINOR}")
    endif()
  endif()
endif()
