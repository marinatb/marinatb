#!/bin/sh

sudo ovs-vsctl --no-wait add-br ovsdpdkbr0 \
  -- set bridge ovsdpdkbr0 datapath_type=netdev

sudo ovs-vsctl --no-wait add-port ovsdpdkbr0 dpdk0 \
  -- set Interface dpdk0 type=dpdk

sudo ovs-vsctl add-port ovsdpdkbr0 vhost-user-1 \
  -- set Interface vhost-user-1 type=dpdkvhostuser

sudo ovs-vsctl add-port ovsdpdkbr0 vhost-user-2 \
  -- set Interface vhost-user-2 type=dpdkvhostuser
