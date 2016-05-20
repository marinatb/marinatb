#!/bin/bash

function tunnel_up {

  ssh -M -S /tmp/$1-$2.sock -f -L $2:localhost:$3 -A ry@marina.deterlab.net \
    ssh -M -S /tmp/$1-$2.sock -A -fnN -L $3:$1:$2 rgoodfel@boss

}

function tunnel_down {

  ssh ry@marina.deterlab.net ssh -S /tmp/$1-$2.sock -O exit rgoodfel@boss
  #ssh -S /tmp/$1-$2.sock -O exit ry@marina.deterlab.net

}


case $3 in 
  "up")
    tunnel_up $2 17988 12348
    tunnel_up $2 17990 12347
    tunnel_up $2 443 12346
    tunnel_up $2 80  22345
  ;;
  "down")
    tunnel_down $2 17988
    tunnel_down $2 17990
    tunnel_down $2 443
    tunnel_down $2 80
  ;;
  *) echo "usage tunnel <node> [up|down]" ;;
esac



