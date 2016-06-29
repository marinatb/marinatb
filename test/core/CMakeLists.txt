#-------------------------------------------------------------------------------
# marinatb-test-common build file
#
# builds executable tests that exercise the materialization service
#
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_executable( core-test
  ../catchme.cxx
  model_embed.cxx
  jsonification.cxx
  net.cxx
  exec.cxx
)

target_link_libraries( core-test
  marina-core
  marinatb-client
  marina-test-models
)
