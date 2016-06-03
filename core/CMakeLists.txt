#-------------------------------------------------------------------------------
# marinatb-core build file
#
# builds the marina testbed core components
#   + access                
#   + accounts             
#   + blueprint         TODO
#   + materialization   TODO
#   + host-control      TODO
#   + node-control      TODO
#
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_library( marina-core
  blueprint.cxx
  db.cxx
  topo.cxx
  util.cxx
  embed.cxx
  compilation.cxx
)

add_library( marina-netlink
  netlink.cxx
)

target_link_libraries( marina-core 
  marinatb-common 
  pq
  uuid
  fmt
)

add_executable( mac2ifname mac2ifname.cxx )
target_link_libraries( mac2ifname marina-netlink )

add_executable( accounts accounts.cxx )
target_link_libraries( accounts 
  marina-core
  proxygenhttpserver 
)

add_executable( access access.cxx )
target_link_libraries( access 
  marina-core
  proxygenhttpserver 
)

add_executable( blueprint blueprint-service.cxx )
target_link_libraries( blueprint 
  marina-core
  proxygenhttpserver 
)

add_executable( materialization materialization-service.cxx )
target_link_libraries( materialization 
  marina-core
  proxygenhttpserver 
)

add_executable( host-control host-control.cxx )
target_link_libraries( host-control
  marina-core
  proxygenhttpserver 
)

add_executable( marina client.cxx )
target_link_libraries( marina marina-core dl )

install(TARGETS marina-core accounts access blueprint materialization host-control
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Ryan Goodfellow")
set(CPACK_DEBIAN_PACKAGE_NAME "marinatb-ops")
set(CPACK_DEBIAN_PACKAGE_VERSION "0.1-ubuntu1")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "marinatb core operations services and database")
set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://marinatb.net")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS "YES")

include(CPack)
