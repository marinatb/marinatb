#-------------------------------------------------------------------------------
# marinatb-test-common build file
#
# builds executable tests that excercise the common code
# components shared among the marinatb software components
#
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_executable( run_common_tests
  ../catchme.cxx
  net/http_client_tests.cxx
  #  net/http_server_tests.cxx 
)

target_link_libraries( run_common_tests
  marinatb-common
)

add_executable( run_server_tests
  ../catchme.cxx
  net/http_server_tests.cxx
)

target_link_libraries( run_server_tests
  marinatb-common
  proxygenhttpserver
)

