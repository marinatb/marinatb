#!/bin/bash

set -u
set -e

srcdir="${SRCDIR:-/tmp}"
jobs_=`nproc`

cxxflags="\
  -stdlib=libc++ -std=c++14 \
  -I/usr/local/include/c++/v1 \
  -Wno-sign-compare \
  -Wno-reserved-user-defined-literal"

export LD_LIBRARY_PATH=/usr/local/lib

cd $srcdir

if [ ! -d "gflags" ]; then
  git clone https://github.com/gflags/gflags.git
fi

cd gflags
mkdir -p build
cd build
cmake \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_FLAGS="$cxxflags" \
  -DGFLAGS_BUILD_SHARED_LIBS=true \
  -DGFLAGS_INSTALL_SHARED_LIBS=true \
  -DGFLAGS_BUILD_STATIC_LIBS=false \
  -DGFLAGS_INSTALL_STATIC_LIBS=false \
  .. \
  -G Ninja
ninja
sudo ninja install
