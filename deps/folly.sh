#!/bin/bash

set -u
set -e

srcdir="${SRCDIR:-/tmp}"
jobs_=`nproc`
config_="--enable-static --enable-shared=NO"

export CXX=clang++
export CC=clang
export CXXFLAGS=" \
  -stdlib=libc++ -std=c++14 \
  -I/usr/local/include/c++/v1 \
  -Wno-sign-compare \
  -Wno-reserved-user-defined-literal -fPIC"
export LDFLAGS="-pthread"

export LD_LIBRARY_PATH=/usr/local/lib

sudo apt-get install -y \
  libssl-dev \
  libevent-dev 

cd $srcdir

#folly
if [ ! -d "folly" ]; then
  git clone https://github.com/facebook/folly
fi

cd folly/folly
autoreconf -i
./configure $config_
make -j$jobs_
sudo make install

