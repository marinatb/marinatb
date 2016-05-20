#!/bin/bash

#(node_id, vnc_port)
function tunnel_up {
  ssh -M -S /tmp/mrvnc-$1-$2.sock -fnN -L 1$2:$1:$2 -A ry@marina.deterlab.net
}

#(node_id, vnc_port)
function tunnel_down {
  ssh -S /tmp/mrvnc-$1-$2.sock -O exit rgoodfel@boss
}

#main(node_id, vnc_port)
case $3 in
  "up")
    tunnel_up $1 $2
  ;;
  "down")
    tunnel_down $1 $2
  ;;
  *) echo "usage: mrtb-node-tunnel <node> <vnc_port> [up|down]"
esac
