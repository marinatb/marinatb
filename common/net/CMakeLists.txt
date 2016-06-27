#-------------------------------------------------------------------------------
# marinatb-common:net build file
#
# builds network communication library libmarinatb-common-net that is used by 
# all the marinatb components that communicate over the network
#
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_library( marinatb-common-net
  http_request.cxx
  http_server.cxx
  proto.cxx
  glog.cxx
)

target_link_libraries( marinatb-common-net
  proxygenlib
  proxygenhttpserver
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

