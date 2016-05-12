#!/bin/bash

set -u
set -e

rm -rf marinatb-ops*

./make_tarball.sh

bzr dh-make marinatb-ops 0.1 marinatb.tgz
cd marinatb-ops/debian
rm *ex *EX

