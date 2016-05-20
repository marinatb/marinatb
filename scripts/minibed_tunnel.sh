#!/bin/bash

case $1 in
  "up")
    ./mrtb-node-tunnel.sh mrtb0 5901 up
    ./mrtb-node-tunnel.sh mrtb0 5902 up
    ./mrtb-node-tunnel.sh mrtb1 5903 up
    ./mrtb-node-tunnel.sh mrtb1 5904 up
  ;;
  "down")
    ./mrtb-node-tunnel.sh mrtb0 5901 down
    ./mrtb-node-tunnel.sh mrtb0 5902 down
    ./mrtb-node-tunnel.sh mrtb1 5903 down
    ./mrtb-node-tunnel.sh mrtb1 5904 down
  ;;
  *) echo "usage: minibed_tunnel [up|down]"
esac
