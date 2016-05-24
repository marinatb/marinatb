#!/bin/bash

case $1 in
  "up")
    ./ipmi-node-tunnel-sm.sh 10.10.0.41 up
    ./ipmi-node-tunnel-sm.sh 10.10.0.42 up
    ./ipmi-node-tunnel-sm.sh 10.10.0.43 up
    ./ipmi-node-tunnel-sm.sh 10.10.0.44 up
  ;;
  "down")
    ./ipmi-node-tunnel-sm.sh 10.10.0.41 down
    ./ipmi-node-tunnel-sm.sh 10.10.0.42 down
    ./ipmi-node-tunnel-sm.sh 10.10.0.43 down
    ./ipmi-node-tunnel-sm.sh 10.10.0.44 down
  ;;
  *) echo "usage: wsu_tunnel [up|down]"
esac
