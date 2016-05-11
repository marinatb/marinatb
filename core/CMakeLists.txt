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

target_link_libraries( marina-core 
  marinatb-common 
  pq
  uuid
)

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

