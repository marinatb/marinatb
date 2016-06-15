#!/bin/bash

set -u
set -e

srcdir="${SRCDIR:-/tmp}"
jobs_=`nproc`

cxxflags="\
  -stdlib=libc++ -std=c++14 \
  -I/usr/local/include/c++/v1"

export LD_LIBRARY_PATH=/usr/local/lib

cd $srcdir

if [ ! -d "fmt" ]; then
  git clone https://github.com/fmtlib/fmt.git
fi

cd fmt
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
