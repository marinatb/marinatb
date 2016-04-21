#-------------------------------------------------------------------------------
# marinatb-test build file
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#cmake .........................................................................
cmake_minimum_required(VERSION 3.0)
project(libdna)
set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  "${libdna_SOURCE_DIR}/cmake"
  "/usr/local/lib/cmake")
