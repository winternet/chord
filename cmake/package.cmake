
set(CPACK_PACKAGE_VENDOR "winternet")
set(CPACK_PACKAGE_CONTACT "winternet@github.com")

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Highly experimental layered distributed peer to peer overlay filesystem based on distributed hashtables (DHTs)."
  CACHE STRING "description of the package metadata"
)
set(CPACK_VERBATIM_VARIABLES YES)

set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME})
set(CPACK_OUTPUT_FILE_PREFIX "${CMAKE_SOURCE_DIR}/packages")

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

#set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/")
#set(CPACK_PACKAGING_INSTALL_PREFIX "/")

find_program(DPKG_FOUND dpkg PATH_SUFFIXES bin)
if(DPKG_FOUND)
  # package name for deb my-app-0.1.0-Linux.deb => my-app_0.1.0_amd64.deb
  set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
  set(CPACK_DEBIAN_PACKAGE_SHLIPDEPS ON)
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "libfuse3-3")
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "winternet")
  set(CPACK_DEB_COMPONENT_INSTALL ON)
endif()

find_program(RPMBUILD_FOUND rpmbuild PATH_SUFFIXES bin)
if(RPMBUILD_FOUND) 
  set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION /bin)
endif()


install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/config/fuse_node0.yml DESTINATION etc/chord/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/config/node0.yml DESTINATION etc/chord/)

include(CPack)
