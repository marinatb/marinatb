#!/bin/bash

function tunnel_up {

  ssh -M -S /tmp/$1-$2.sock -p 10007 -fnN -L $2:$1:$2 ry@border-fw.gridstat.net 

}

function tunnel_down {

  ssh -S /tmp/$1-$2.sock -O exit ry@border-fw.gridstat.net

}


case $2 in 
  "up")
    tunnel_up $1 443
    tunnel_up $1 80
  ;;
  "down")
    tunnel_down $1 443
    tunnel_down $1 80
  ;;
  *) echo "usage tunnel <node> [up|down]" ;;
esac



