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
      cnet                    -- setup the control net

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

if [ -z "${UUID}" ]; then
    UUID=`git show --quiet --pretty=%h`
else
    UUID=${UUID//\//_}
fi

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
  docker exec -i -t ${UUID}-${1} /bin/bash
}

function do_run_console {
   $RUN $ARGS --hostname=${1} --name=${UUID}-${1} --net=${UUID} --net-alias=${1} \
     -v `pwd`:/code \
     --entrypoint=bash \
     --dns=192.168.47.1 \
     ${1}:${UUID}
}

function do_build_console {
   $RUN $ARGS --hostname=builder --name=builder --net=${UUID} \
     -v `pwd`:/code \
     --entrypoint=bash marinatb/builder
}

#unused?
function do_run {
  TGT=$1
  shift
  $RUN $ARGS --hostname=$TGT --name=${UUID}-$TGT --net=${UUID} --net-alias=${TGT} \
    -v `pwd`:/code \
    ${UUID}-$TGT $@
}

function do_launch {
  $RUN $DARGS --hostname=${1} --name=${UUID}-${1} --net=${UUID} --net-alias=${1} -v `pwd`:/code --dns=192.168.47.1 ${1}:${UUID}
}

function do_terminate {
  docker stop -t 0 ${UUID}-${1}
  docker rm ${UUID}-${1}
}

function do_restart {
  do_terminate ${UUID}-${1}
  do_launch ${UUID}-${1}
}

function do_containerize {
  docker build -t "${1}:${UUID}" -f "${1}.dock" .
}

#
#function create_net {
#  docker network create --driver bridge \
#  --subnet=192.168.47.0/24 \
#  --ip-range=192.168.47.64/26 \
#  tnet
#}
#

#
function create_net {
  docker network create --driver bridge ${UUID}
}
#

function do_cnet {
  cp /etc/hosts /tmp/hosts
  cp /tmp/hosts /tmp/hosts.bak
  dbaddr=`docker inspect \
    --format='{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' \
    db`
  grep -q '.*db' /tmp/hosts && \
    sed -i "s/.*db/$dbaddr db/" /tmp/hosts || echo "$dbaddr db" >> /tmp/hosts
  sudo cp /tmp/hosts /etc/hosts
}

case $1 in
  "containerize") do_containerize $2 ;;
  "cmake") do_cmake ;;
  "ninja") shift; do_ninja $@ ;;
  "build-console") do_build_console $2 ;;
  "console") do_console $2 ;;
  "run-console") do_run_console $2 ;;
  "run") do_run $2 ;;
  "net")  create_net ;;
  "cnet") do_cnet ;;
  "launch") do_launch $2 ;;
  "terminate") do_terminate $2 ;;
  "restart") do_restart $2 ;;
  "containerize-system")
    do_containerize db
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
    cp build/core/mac2ifname host-pkg/bin/

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
