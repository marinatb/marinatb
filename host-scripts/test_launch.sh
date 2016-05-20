#!/bin/bash

sudo qemu-system-x86_64 \
  --enable-kvm \
  -cpu IvyBridge -smp 4,sockets=2,cores=2,threads=1 \
  -m 2048 -mem-path /dev/hugepages -mem-prealloc \
  -object memory-backend-file,id=mem0,size=2048M,mem-path=/dev/hugepages,share=on \
  -numa node,memdev=mem0 -mem-prealloc \
  -hda /space/images/std/ubuntu-server-xenial.qcow2 \
  -chardev socket,id=chr0,path=/var/run/openvswitch/vhost-user-1 \
  -netdev type=vhost-user,id=net0,chardev=chr0,vhostforce \
  -device virtio-net-pci,mac=00:00:00:00:00:01,netdev=net0 \
  -vnc 0.0.0.0:1,password \
  -monitor stdio 
  

  #-csum=off,gso=off,guest_tso4=off,guest_tso6=off,guest_ecn=off \
