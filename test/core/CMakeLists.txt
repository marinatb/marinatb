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
  marinatb-common
  marina-core
  marina-test-models
)
