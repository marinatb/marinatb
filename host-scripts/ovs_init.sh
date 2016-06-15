#!/bin/sh

#virtual side bridge
sudo ovs-vsctl add-br mrtb-vbr0 \
  -- set bridge mrtb-vbr0 datapath_type=netdev

sudo ovs-vsctl add-port mrtb-vbr0 vhost-user-1 \
  -- set Interface vhost-user-1 type=dpdkvhostuser

sudo ovs-vsctl add-port mrtb-vbr0 vhost-user-2 \
  -- set Interface vhost-user-2 type=dpdkvhostuser

sudo ovs-vsctl add-port mrtb-vbr0 vxlan0 \
  -- set Interface vxlan0 type=vxlan \
     options:remote_ip=192.168.247.2

#physical side bridge
sudo ovs-vsctl  add-br mrtb-pbr0 \
  -- set bridge mrtb-pbr0 datapath_type=netdev \
     other_config:hwaddr="00:1b:21:cd:a0:0c"

sudo ovs-vsctl  add-port mrtb-pbr0 dpdk0 \
  -- set Interface dpdk0 type=dpdk

sudo ip addr add 192.168.247.1/24 dev mrtb-pbr0
sudo ip link set mrtb-pbr0 up
sudo iptables -F

