#!/bin/bash

set -u
set -e

srcdir="${SRCDIR:-/tmp}"
jobs_=`nproc`

url=https://sourceforge.net/projects/boost/files/boost/1.60.0/boost_1_60_0.tar.gz/download

cxxflags_="-stdlib=libc++ -std=c++1z -I/usr/local/include/c++/v1"
linkflags_="-stdlib=libc++"

cd $srcdir

if [ ! -d "boost_1_60_0" ]; then
  curl -OL $url -o 
  mv download > boost_1_60_0.tgz
  tar zxf boost_1_60_0.tgz
fi

sudo apt-get install -y libbz2-dev

cd boost_1_60_0
./bootstrap.sh --with-toolset=clang
./b2 \
  toolset=clang \
  cxxflags="$cxxflags_" \
  linkflags="$linkflags_" \
  --without-python -j$jobs_

sudo ./b2 install \
  toolset=clang \
  cxxflags="$cxxflags_" \
  linkflags="$linkflags_" \
  --without-python

