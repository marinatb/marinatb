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

export LD_LIBRARY_PATH=/usr/local/lib

cd $srcdir

#glog
if [ ! -d "glog" ]; then
  #git clone https://github.com/google/glog.git
  git clone https://github.com/rcgoodfellow/glog.git
fi

cd glog
autoreconf -i
./configure $config_
make -j$jobs_
sudo make install

