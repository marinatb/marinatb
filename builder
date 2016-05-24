#!/bin/bash

read -d '' USAGE <<- "EOF"
  usage: builder <cmd>
  cmd:
    node-level:
      containerize <target>   -- build the container for the component 'target'
      console <target>        -- get a console in a running target
      run-console <target>    -- launch 'target' and run a console in it 
      launch <target>         -- launch 'target'
      terminate <target>      -- terminate a running 'target'
      restart <target>        -- terminate and launch a running 'target'

    system-level:
      containerize-system     -- build all system containers
      launch-system           -- launch the whole system
      terminate-system        -- terminate the whole system
      restart-system          -- restart the whole system
      net                     -- create the ops network

    development:
      pkg                     -- package up all deps of the core ops components.
      host-pkg                -- create a host-control package (host-pkg.tgz)
      deploy                  -- deploy host-pkg to the host nodes and unpack
EOF

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
     --entrypoint=bash \
     --dns=192.168.47.1 \
     $1
}

function do_build_console {
   $RUN $ARGS --hostname=builder --name=builder --net=tnet \
     -v `pwd`:/code \
     --entrypoint=bash marinatb/builder
}

#unused?
function do_run {
  TGT=$1
  shift
  $RUN $ARGS --hostname=$TGT --name=$TGT --net=tnet \
    -v `pwd`:/code \
    $TGT $@
}

function do_launch {
  $RUN $DARGS \
    --hostname=$1 --name=$1 --net=tnet \
    -v `pwd`:/code \
    --dns=192.168.47.1 \
    $1
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
    rm -rf pkg
    mkdir -p pkg/usr/local/lib
    #docker cp builder:/usr/lib pkg/usr/
    ldd build/core/materialization |\
    awk 'NF == 4 {print $3}; NF == 2 {print $1}' |\
    grep local |\
    xargs cp -t pkg/usr/local/lib/

    #mkdir -p pkg/usr/local
    #docker cp builder:/usr/local/lib pkg/usr/local/
    #cp -r /usr/local/lib pkg/usr/local
    #these monsters are only needed on the builder atm
    #rm pkg/usr/local/lib/libclang*
    #rm pkg/usr/local/lib/libLLVM*
    #rm pkg/usr/local/lib/libLTO.so
  ;;

  "host-pkg")
    rm -rf host-pkg
    mkdir -p host-pkg/libs
    mkdir -p host-pkg/bin

    ldd build/core/host-control |\
    awk 'NF == 4 {print $3}; NF == 2 {print $1}' |\
    grep local |\
    xargs cp -t host-pkg/libs/

    cp build/core/host-control host-pkg/bin/

    tar czf host-pkg.tgz host-pkg
    rm -rf host-pkg
  ;;

"deploy")
    echo "Deploying host packages"
    scp host-pkg.tgz murphy@mrtb0:~/
    scp host-pkg.tgz murphy@mrtb1:~/

    echo "Unpacking host packages on hosts"
    ssh murphy@mrtb0 "tar xzf host-pkg.tgz"
    ssh murphy@mrtb1 "tar xzf host-pkg.tgz"
  ;;

  *) echo "$USAGE" ;;
esac

