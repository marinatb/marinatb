#-------------------------------------------------------------------------------
# marinatb-common build file
#
# !!!:
# This build file is designed to be used in conjunction with the marina-builder
# build file @ github.com/marinatb/builder
#
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_subdirectory(net)

#add_library( marinatb-common version.cxx $<TARGET_OBJECTS:marinatb-common-net>)
add_library( marinatb-common version.cxx )
target_link_libraries( marinatb-common
  marinatb-common-net
)
