#-------------------------------------------------------------------------------
# marinatb-test build file
#
# build a library that contains the MarinaTB test blueprints
#
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_library(marina-test-models

  #networked system blueprints
  blueprints/mars.cxx
  blueprints/hello-marina.cxx

  #testbed topologies
  topos/deter2015.cxx
  topos/minibed.cxx

)

target_link_libraries(marina-test-models marina-core)
