#!/bin/bash

set -u
set -e

srcdir="${SRCDIR:-/tmp}"
jobs_=`nproc`

export CXX=clang++
export CC=clang
export CXXFLAGS=" \
  -stdlib=libc++ -std=c++14 \
  -I/usr/local/include/c++/v1 \
  -Wno-sign-compare \
  -Wno-reserved-user-defined-literal -fPIC"
export LDFLAGS="-pthread -lc++abi"

export LD_LIBRARY_PATH=/usr/local/lib

cd $srcdir

sudo apt-get install -y \
  autoconf-archive \
  libcap-dev \
  gperf


#proxygen
if [ ! -d "proxygen" ]; then
  #git clone https://github.com/facebook/proxygen.git
  git clone https://github.com/rcgoodfellow/proxygen.git
fi

cd proxygen/proxygen
git checkout fix
if [ ! -f "configure" ]; then
  autoreconf -ivf
fi
./configure
make -j$jobs_
sudo make install

