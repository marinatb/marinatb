#!/bin/bash

ARGS="-ti --rm --hostname=builder --name=builder --privileged"
CMAKE="cmake"
CMAKE_ARGS=".. -G Ninja"
CONTAINER="builder"
RUN="docker run"

function do_cmake {
  mkdir -p build
  $RUN $ARGS \
    -v `pwd`:/code \
    --workdir=/code/build \
    --entrypoint=$CMAKE $CONTAINER $CMAKE_ARGS
}

function do_ninja {
  $RUN $ARGS \
    -v `pwd`:/code \
    --workdir=/code/build \
    --entrypoint=$NINJA $CONTAINER $@
}

function do_run {
  $RUN $ARGS \
    -v `pwd`:/code \
    --entrypoint=bash $CONTAINER
}

case $1 in
  "cmake") do_cmake;;
  "ninja") shift; do_ninja $@;;
  "run") do_run;;
  *) echo "usage build <component> [cmake|ninja|test]" ;;
esac
