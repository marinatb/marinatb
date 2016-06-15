#!/bin/bash

sudo apt-get install openvswitch-switch openvswitch-switch-dpdk

sudo update-alternatives --set \
  ovs-vswitchd \
  /usr/lib/openvswitch-switch-dpdk/ovs-vswitchd-dpdk


echo "DPDK_OPTS='--dpdk -c 0x1 -n 4 -m 2048 --vhost-owner libvirt-qemu:kvm --vhost-perm 0664'" \
| sudo tee -a /etc/default/openvswitch-switch

sudo service openvswitch-switch restart
