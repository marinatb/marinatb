#!/bin/bash

set -u
set -e

srcdir="${SRCDIR:-/tmp}"
jobs_=`nproc`
config_="--enable-static"

export CXX=clang++
export CC=clang
export CXXFLAGS=" \
  -stdlib=libc++ -std=c++14 \
  -I/usr/local/include/c++/v1 \
  -Wno-sign-compare \
  -Wno-reserved-user-defined-literal"

cxxflags="\
  -stdlib=libc++ -std=c++14 \
  -I/usr/local/include/c++/v1 \
  -Wno-sign-compare \
  -Wno-reserved-user-defined-literal"

export LD_LIBRARY_PATH=/usr/local/lib

cd $srcdir

if [ ! -d "double-conversion" ]; then
  git clone https://github.com/google/double-conversion.git
fi

cd double-conversion
mkdir -p build
cd build
cmake \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_FLAGS="$cxxflags" \
  .. \
  -G Ninja
ninja
sudo ninja install
