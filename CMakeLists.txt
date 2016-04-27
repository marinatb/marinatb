#-------------------------------------------------------------------------------
# marinatb-test build file
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# cmake ........................................................................
cmake_minimum_required(VERSION 3.0)

set( CMAKE_CXX_COMPILER /usr/bin/clang++ )
set( CMAKE_C_COMPILER /usr/bin/clang )

project(marinatb)

include_directories(/usr/local/c++/v1)


set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  "${marinatb_SOURCE_DIR}/cmake"
  "/usr/local/lib/cmake")


# compiler settings ............................................................
set( CMAKE_C_FLAGS
  "${CMAKE_C_FLAGS} \
  -std=c11 \
  -Wall -Wextra " )

set( CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")

set( CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} \
  -g -O0 -fno-omit-frame-pointer" )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
  -g -std=c++14 -stdlib=libc++ \
  -Wall -Wextra \
  -fdiagnostics-color=always" )

set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} \
  -O3" )

set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} \
  -g -O0 -fno-omit-frame-pointer" )

include_directories(
  "${marinatb_SOURCE_DIR}"
  "/usr/local/include/c++/v1"
)

#3p ............................................................................
#if(EXISTS "${marinatb_SOURCE_DIR}/3p/proxygen")
#  add_subdirectory(3p/proxygen/proxygen)
#endif(EXISTS "${marinatb_SOURCE_DIR}/3p/proxygen")
if(EXISTS "${marinatb_SOURCE_DIR}/3p/json")
  add_subdirectory(3p/json)
endif(EXISTS "${marinatb_SOURCE_DIR}/3p/json")

# build components .............................................................
if(EXISTS "${marinatb_SOURCE_DIR}/common")
  add_subdirectory(common)
endif(EXISTS "${marinatb_SOURCE_DIR}/common")

if(EXISTS "${marinatb_SOURCE_DIR}/test")
  add_subdirectory(test)
endif(EXISTS "${marinatb_SOURCE_DIR}/test")

if(EXISTS "${marinatb_SOURCE_DIR}/api")
  add_subdirectory(api)
endif(EXISTS "${marinatb_SOURCE_DIR}/api")

if(EXISTS "${marinatb_SOURCE_DIR}/core")
  add_subdirectory(core)
endif(EXISTS "${marinatb_SOURCE_DIR}/core")

