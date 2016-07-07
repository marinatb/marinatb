#-------------------------------------------------------------------------------
# marinatb-test-common build file
#
# builds executable tests that exercise the api
#
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_executable( run_api_tests
  ../catchme.cxx
  api_test.cxx
  blueprint.cxx
  materialization.cxx
)

target_link_libraries( run_api_tests
  marina-core
  marinatb-client
  marina-test-models
)
