#-------------------------------------------------------------------------------
# marinatb-api build file
#
# builds the marina testbed api implementation
#
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_executable( marina-api api.cxx )
target_link_libraries( marina-api marinatb-common proxygenhttpserver )
