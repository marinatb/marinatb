#!/bin/bash

ARGS="-ti --rm --privileged"
CMAKE="cmake"
CMAKE_ARGS=".. -G Ninja"
CONTAINER="builder"
RUN="docker run"

function do_cmake {
  mkdir -p build
  $RUN $ARGS --hostname=builder --name=builder \
    -v `pwd`:/code \
    --workdir=/code/build \
    --entrypoint=$CMAKE $CONTAINER $CMAKE_ARGS
}

function do_ninja {
  $RUN $ARGS --hostname=builder --name=builder \
    -v `pwd`:/code \
    --workdir=/code/build \
    --entrypoint=$NINJA $CONTAINER $@
}

function do_run {
  $RUN $ARGS --hostname=$1 --name=$1 \
    -v `pwd`:/code \
    --entrypoint=bash $1
}

case $1 in
  "containerize") docker build -t "$2" -f "${2}.dock" . ;;
  "cmake") do_cmake ;;
  "ninja") shift; do_ninja $@ ;;
  "run") do_run $2 ;;

  #yes this is gross~~ but proxygen has a ghetto build system and no packaging
  #so we need to work around that for the time being, perhaps contribute a
  #build environment back to fb
  "pkg")
    #mkdir -p pkg/usr
    #docker cp builder:/usr/lib pkg/usr/

    mkdir -p pkg/usr/local
    docker cp builder:/usr/local/lib pkg/usr/local/
  ;;

  *) echo "usage build <component> [cmake|ninja|test]" ;;
esac
