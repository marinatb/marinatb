#!/bin/bash

#setup linux with hugepages
sudo cp config/etc/default/grub /etc/default/
sudo update-grub
