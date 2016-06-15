#!/bin/bash

#(node_id, vnc_port)
function tunnel_up {
  ssh -M -S /tmp/mrvnc-$1-$2.sock -fnN -L $2:$1:$3 ry@marina.deterlab.net
}

#(node_id, vnc_port)
function tunnel_down {
  ssh -S /tmp/mrvnc-$1-$2.sock -O exit ry@marina.deterlab.net
}

#main(node_id, vnc_port)
case $4 in
  "up")
    tunnel_up $1 $2 $3
  ;;
  "down")
    tunnel_down $1 $2
  ;;
  *) echo "usage: mrtb-node-tunnel <node> <vnc_port> [up|down]"
esac
