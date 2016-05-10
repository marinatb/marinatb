#!/bin/bash

ARGS="-ti --rm --privileged"
DARGS="-d --privileged"
CMAKE="cmake"
CMAKE_ARGS=".. -G Ninja -DCMAKE_BUILD_TYPE=Debug"
CONTAINER="marinatb/builder"
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

function do_console {
  docker exec -i -t $1 /bin/bash
}

function do_run_console {
   $RUN $ARGS --hostname=$1 --name=$1 --net=tnet \
     -v `pwd`:/code \
     --entrypoint=bash $1
}

function do_build_console {
   $RUN $ARGS --hostname=builder --name=builder --net=tnet \
     -v `pwd`:/code \
     --entrypoint=bash marinatb/builder
}

function do_run {
  TGT=$1
  shift
  $RUN $ARGS --hostname=$TGT --name=$TGT --net=tnet \
    -v `pwd`:/code \
    $TGT $@
}

function do_launch {
  $RUN $DARGS --hostname=$1 --name=$1 --net=tnet -v `pwd`:/code $1
}

function do_terminate {
  docker stop -t 0 $1
  docker rm $1
}

function do_restart {
  do_terminate $1
  do_launch $1
}

function do_containerize {
  docker build -t "$1" -f "${1}.dock" . 
}

case $1 in
  "containerize") do_containerize $2 ;;
  "cmake") do_cmake ;;
  "ninja") shift; do_ninja $@ ;;
  "build-console") do_build_console $2 ;;
  "console") do_console $2 ;;
  "run-console") do_run_console $2 ;;
  "run") do_run $2 ;;
  "net") docker network create --driver bridge tnet ;;
  "launch") do_launch $2 ;;
  "terminate") do_terminate $2 ;;
  "restart") do_restart $2 ;;
  "containerize-system")
    do_containerize api
    do_containerize access
    do_containerize accounts
    do_containerize blueprint
    do_containerize materialization
    ;;
  "launch-system")
    do_launch db
    do_launch api
    do_launch access
    do_launch accounts
    do_launch blueprint
    do_launch materialization
    ;;
  "terminate-system")
    do_terminate api
    do_terminate access
    do_terminate accounts
    do_terminate blueprint
    do_terminate materialization
    do_terminate db
    ;;
  "restart-system")
    do_restart db
    do_restart api
    do_restart access
    do_restart accounts
    do_restart blueprint
    do_restart materialization
    ;;

  #yes this is gross~~ but proxygen has a ghetto build system and no packaging
  #so we need to work around that for the time being, perhaps contribute a
  #build environment back to fb
  "pkg")
    #mkdir -p pkg/usr
    #docker cp builder:/usr/lib pkg/usr/

    mkdir -p pkg/usr/local
    docker cp builder:/usr/local/lib pkg/usr/local/
    #these monsters are only needed on the builder atm
    rm pkg/usr/local/lib/libclang*
    rm pkg/usr/local/lib/libLLVM*
    rm pkg/usr/local/lib/libLTO.so
  ;;

  *) echo "usage build <component> [cmake|ninja|test]" ;;
esac

