#-------------------------------------------------------------------------------
# marinatb-test build file
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# cmake ........................................................................
cmake_minimum_required(VERSION 3.0)
project(marinatb)
set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  "${libdna_SOURCE_DIR}/cmake"
  "/usr/local/lib/cmake")

# compiler settings ............................................................
set( CMAKE_C_FLAGS
  "${CMAKE_C_FLAGS} \
  -std=c11 \
  -Wall -Wextra \
  -fcolor-diagnostics" )

set( CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")

set( CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} \
  -g -O0 -fno-omit-frame-pointer" )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
  -g -std=c++14 \
  -Wall -Wextra" )

set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} \
  -O3" )

set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} \
  -g -O0 -fno-omit-frame-pointer" )

include_directories(
  "${libdna_SOURCE_DIR}/lib"
  "/usr/local/include/c++/v1"
)

# build components .............................................................
if(EXISTS "${marinatb_SOURCE_DIR}/common")
  add_subdirectory(common)
endif(EXISTS "${marinatb_SOURCE_DIR}/common")

if(EXISTS "${marinatb_SOURCE_DIR}/test")
  add_subdirectory(test)
endif(EXISTS "${marinatb_SOURCE_DIR}/test")

