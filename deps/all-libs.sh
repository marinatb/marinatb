#!/bin/bash

set -u
set -e

./ubuntu-libs.sh
./boost.sh
./double-conversion.sh
./gflags.sh
./glog.sh
./folly.sh
./wangle.sh
./proxygen.sh
./fmt.sh

