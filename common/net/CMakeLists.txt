#-------------------------------------------------------------------------------
# marinatb-common:net build file
#
# builds network communication library libmarinatb-common-net that is used by 
# all the marinatb components that communicate over the network
#
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_library( marinatb-common-net OBJECT
  http_request.cxx
  http_server.cxx
  proto.cxx
  glog.cxx
)

