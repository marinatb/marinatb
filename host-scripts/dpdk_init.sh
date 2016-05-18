#!/bin/bash

sudo apt-get install dpdk dpdk-dev dpdk-doc
#sudo modprobe uio_pci_generic
sudo modprobe vfio-pci

#this will create /dev/uio0 which needs to be bound to an openvswitch port
#sudo dpdk_nic_bind --bind=uio_pic_generic ens2f0
sudo dpdk_nic_bind --bind=vfio-pci ens2f0


