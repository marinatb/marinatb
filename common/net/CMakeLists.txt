#-------------------------------------------------------------------------------
# marinatb-common:net build file
#
# builds network communication library libmarinatb-common-net that is used by 
# all the marinatb components that communicate over the network
#
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_library( marinatb-common-net
  proto.cxx
  glog.cxx
)
target_link_libraries( marinatb-common-net
  wangle
  folly
  double-conversion
  z
  ssl
  crypto
  dl
  cap
  event
  gflags
  glog
  c++abi
  pthread
)

add_library( marinatb-client
  http_request.cxx
)
target_link_libraries( marinatb-client
  marinatb-common-net
  proxygenlib
)

add_library( marinatb-server
  http_server.cxx
)
target_link_libraries( marinatb-server
  marinatb-client
  proxygenhttpserver
)

